#include "ActivateText.h"

#include "AeHookBytes.h"
#include "HookUtil.h"
#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"

// ACTI/FLOR/FURN ATTX (activate prompt, index 1, keyed by the base form ID).
//
// Fallout 4 AE stores the text in a global hash map and OG has no known address for that map,
// so this follows FallHook 1.10.163 (src/110163/RuntimeActivationContextHook.cpp and
// RuntimeHudRolloverHook.cpp + RuntimeHudRolloverActionPatch.cpp). Two hooks:
//   1. TESObjectREFR::GetActivateText runs when the player looks at a reference; remember its base form.
//   2. HUDRollover::ShowRollover shows the prompt; while it runs, the action text slot of its
//      parameter block is pointed at the translated string, then put back.
//
// Hook method: both are prologue hooks (HookUtil::InstallPrologueHook), the same method FallHook uses on
// both IDs. HookUtil has no call-site hook for these because no OG call-site address is known, so prologue
// is the only supported method. A conflict is possible if another mod copies the same prologue
// (po3 Floating Subtitles crashed that way on a different function). HookPrologue refuses to install when
// the first bytes contain a relative instruction (call/jmp/RIP-relative), which covers a prologue another
// mod already jump-patched.
namespace
{
	constexpr auto kTypes = std::to_array({
		DSD::TypeOf("ACTI ATTX"),
		DSD::TypeOf("FLOR ATTX"),
		DSD::TypeOf("FURN ATTX"),
	});

	// ShowRolloverParameters +0x08 holds the primary action text (a BSFixedString entry pointer).
	// Offset from FallHook RuntimeHudRolloverActionPatch.cpp kPrimaryActionSlot.
	constexpr std::size_t kActionSlot = 0x08;

	// FallHook uses the same 1000 ms window between GetActivateText and ShowRollover
	constexpr std::uint32_t kMaxContextAgeMs = 1000;

	struct Tables
	{
		// Base form ID -> BSFixedStringCS entry pointer (the entries are kept alive by a leaked vector, see Apply)
		boost::unordered_flat_map<std::uint32_t, const void*> entries;
	};

	// Replaced as a whole, so hooks on other threads never see a half-built table
	std::atomic<std::shared_ptr<const Tables>> g_tables;

	// Latest looked-at base form: low 32 bits = base form ID, high 32 bits = GetTickCount64 low bits.
	// One atomic word, so the two hooks (possibly on different threads) never read a torn pair.
	std::atomic<std::uint64_t> g_lastLook{ 0 };

	// TESObjectREFR::GetActivateText(BSString& out), OG Address Library ID 1077221
	// (ID and signature from FallHook src/110163/RuntimeActivationContextHook.cpp, kGetActivateTextID).
	struct GetActivateText
	{
		static bool thunk(RE::TESObjectREFR* a_this, RE::BSStringT<char>& a_out)
		{
			const auto result = func(a_this, a_out);

			const auto base = a_this ? a_this->GetObjectReference() : nullptr;
			const auto baseID = base ? static_cast<std::uint32_t>(base->GetFormID()) : 0u;
			const auto tick = static_cast<std::uint32_t>(::GetTickCount64());
			g_lastLook.store((static_cast<std::uint64_t>(tick) << 32) | baseID, std::memory_order_release);
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// HUDRollover::ShowRollover(void* rollover, void* parameters), OG Address Library ID 414044
	// (ID and signature from FallHook src/110163/RuntimeHudRolloverHook.cpp, kShowRolloverID).
	struct ShowRollover
	{
		static void thunk(void* a_this, void* a_parameters)
		{
			// Runs while the crosshair is on something, so the common case is two cheap checks: no recent look, or no translation.
			// The parameter block belongs to the caller and is always writable, so no memory query is needed.
			const void** slot = nullptr;
			const void*  original = nullptr;

			const auto look = g_lastLook.load(std::memory_order_acquire);
			const auto age = static_cast<std::uint32_t>(::GetTickCount64()) - static_cast<std::uint32_t>(look >> 32);
			if (a_parameters && age <= kMaxContextAgeMs) {
				const auto tables = g_tables.load(std::memory_order_acquire);
				const auto it = tables ? tables->entries.find(static_cast<std::uint32_t>(look)) : decltype(tables->entries)::const_iterator{};
				if (tables && it != tables->entries.end()) {
					const auto field = reinterpret_cast<const void**>(static_cast<std::byte*>(a_parameters) + kActionSlot);
					// Only replace a slot that already holds a string, as FallHook does
					if (*field && reinterpret_cast<std::uintptr_t>(*field) >= 0x10000) {
						slot = field;
						original = *field;
						*field = it->second;
					}
				}
			}

			func(a_this, a_parameters);

			if (slot) {
				*slot = original;
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
}

namespace ActivateText
{
	void Install()
	{
		if (!Settings::GetSingleton().activateTextHooks) {
			logger::info("bActivateTextHooks = false: crosshair prompt hooks not installed");
			return;
		}
		if (DSD::GetRuntime() == DSD::Runtime::AE) {
			// AE IDs and the expected bytes: docs/ae-crosscheck.md (GetActivateText, ShowRollover rows). Each hook is checked
			// on its own, so one mismatch leaves the other installed.
			HookUtil::InstallPrologueHookChecked<GetActivateText>(2201128, "TESObjectREFR::GetActivateText", AeHookBytes::kGetActivateText);
			HookUtil::InstallPrologueHookChecked<ShowRollover>(2221994, "HUDRollover::ShowRollover", AeHookBytes::kShowRollover);
			return;
		}
		HookUtil::InstallPrologueHook<GetActivateText>(1077221, "TESObjectREFR::GetActivateText");
		HookUtil::InstallPrologueHook<ShowRollover>(414044, "HUDRollover::ShowRollover");
	}

	void Apply()
	{
		const auto& store = Manager::GetSingleton().Store();
		// The game string pool entries behind the translated prompts. Intentionally never freed: a hook may
		// still use the previous table after a reload, and releasing them during static destruction at process
		// exit would touch the already torn down string pool. One small vector per Apply (once per data load).
		const auto keepAlive = new std::vector<RE::BSFixedStringCS>();
		auto tables = std::make_shared<Tables>();

		for (const auto type : kTypes) {
			std::size_t applied = 0;
			std::size_t formMissing = 0;
			std::size_t wrongType = 0;
			store.ForEach(type, [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry& a_entry) {
				const auto form = DSD::LookupFormByID(a_formID);
				if (!form) {
					++formMissing;
					logger::debug("{} {:08X}: form not found", DSD::TypeCodeToString(type), a_formID);
					return;
				}
				// TESFlora and TESFurniture derive from TESObjectACTI. On AE the RTTI type descriptors of the fork are
				// not usable (their IDs are the 1.10.98x ones), so the form type byte is checked instead: ACTI 0x1B,
				// FLOR 0x29, FURN 0x2A, read from the AE form type name table (docs/ae-crosscheck.md).
				const auto formType = form->GetFormType();
				const bool isActivatable = DSD::GetRuntime() == DSD::Runtime::AE ?
				                               (formType == RE::ENUM_FORM_ID::kACTI || formType == RE::ENUM_FORM_ID::kFLOR || formType == RE::ENUM_FORM_ID::kFURN) :
				                               form->As<RE::TESObjectACTI>() != nullptr;
				if (!isActivatable) {
					++wrongType;
					logger::debug("{} {:08X}: form type {} has no activate text", DSD::TypeCodeToString(type), a_formID, form->GetFormTypeString());
					return;
				}
				auto& fixed = keepAlive->emplace_back(a_entry.text.c_str());
				tables->entries.insert_or_assign(a_formID, *reinterpret_cast<const void* const*>(std::addressof(fixed)));
				++applied;
			});
			logger::info("{} : {:>6} activate texts ready, {:>4} form missing, {:>4} wrong type",
				DSD::TypeCodeToString(type), applied, formMissing, wrongType);
		}

		g_tables.store(std::move(tables), std::memory_order_release);
	}
}
