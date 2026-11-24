#include "LeveledListNames.h"

#include "AeHookBytes.h"
#include "HookUtil.h"
#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"

// LVLI ONAM (override name of a leveled item list, index ignored, keyed by the list's form ID).
//
// Fallout 4 keeps the text in a global BSTHashMap<TESForm*, BSFixedString> with no known OG Address Library ID,
// so this replaces the one reader: slot 0x0A (GetOverrideName) of the second vtable of TESLevItem, the
// TESLeveledList base at +0x68. The game calls it from CalculateCurrentFormList when no name was passed in and
// copies the text onto the generated items. Vtable slot, base offset and the single caller come from the 1.11.191
// disassembly; the 1.10.163 behaviour is confirmed in game only. On AE the slot write is guarded by
// HookUtil::VtableSlotIsSafe (RTTI name, locator offset, slot inside .text, expected function bytes).
namespace
{
	// TESLevItem::TESLeveledList base subobject offset
	constexpr std::ptrdiff_t kLeveledListOffset = 0x68;
	constexpr std::size_t    kGetOverrideNameSlot = 0x0A;

	struct Table
	{
		// List form ID -> translation. Never changed after it is published, and never freed (see Apply),
		// so a c_str handed to the game stays valid for the whole session.
		boost::unordered_flat_map<std::uint32_t, std::string> names;
	};

	std::atomic<const Table*> g_table{ nullptr };
	bool                      g_installed = false;

	// TESLeveledList::GetOverrideName, called with this = the base subobject
	struct GetOverrideName
	{
		static const char* thunk(RE::TESLeveledList* a_this)
		{
			const auto result = func(a_this);
			if (!result || !*result) {  // almost every call: no name, nothing else to do
				return result;
			}

			const auto table = g_table.load(std::memory_order_acquire);
			const auto formID = reinterpret_cast<const RE::TESForm*>(reinterpret_cast<const std::byte*>(a_this) - kLeveledListOffset)->GetFormID();
			const auto it = table ? table->names.find(static_cast<std::uint32_t>(formID)) : decltype(table->names)::const_iterator{};
			const auto translated = (table && it != table->names.end()) ? it->second.c_str() : result;

			if (Settings::GetSingleton().logOverrideNames) {
				logger::info("[LeveledListNames] {:08X}: \"{}\" -> \"{}\"", formID, result, translated);
			}
			return translated;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
}

namespace LeveledListNames
{
	void Apply()
	{
		constexpr auto type = DSD::TypeOf("LVLI ONAM");
		const auto&    store = Manager::GetSingleton().Store();

		// Built in full before it is published. A reload publishes a new table; the old one is leaked on purpose,
		// because a pointer returned from the old table may still be held by the game.
		auto        table = std::make_unique<Table>();
		std::size_t rows = 0;
		std::size_t formMissing = 0;
		std::size_t wrongType = 0;
		store.ForEach(type, [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry& a_entry) {
			++rows;
			const auto form = DSD::LookupFormByID(a_formID);
			if (!form) {
				++formMissing;
				logger::debug("LVLI ONAM {:08X}: form not found", a_formID);
				return;
			}
			// AE: the fork's RTTI IDs are the 1.10.98x ones, so check the form type byte (LVLI 0x38, AE form type name table)
			const bool isLeveledItem = DSD::GetRuntime() == DSD::Runtime::AE ?
			                               form->GetFormType() == RE::ENUM_FORM_ID::kLVLI :
			                               form->As<RE::TESLevItem>() != nullptr;
			if (!isLeveledItem) {
				++wrongType;
				logger::debug("LVLI ONAM {:08X}: form type {} is not a leveled item list", a_formID, form->GetFormTypeString());
				return;
			}
			table->names.insert_or_assign(a_formID, a_entry.text);
		});

		const auto found = table->names.size();
		logger::info("LVLI ONAM : {:>6} rows, {:>4} forms found, {:>4} form missing, {:>4} wrong type", rows, found, formMissing, wrongType);

		if (!found) {
			return;  // nothing to translate: no hook, no cost
		}
		g_table.store(table.release(), std::memory_order_release);

		if (!std::exchange(g_installed, true)) {
			// The table is published above, so the hook never sees an empty one.
			// The original is stored before the slot changes, so a call on another thread never finds func unset.
			REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::TESLevItem[1] };
			if (DSD::GetRuntime() == DSD::Runtime::AE &&
				!HookUtil::VtableSlotIsSafe(vtable.address(), kGetOverrideNameSlot, ".?AVTESLevItem@@", static_cast<std::uint32_t>(kLeveledListOffset),
					AeHookBytes::kLeveledListGetOverrideName, "TESLevItem::GetOverrideName")) {
				return;  // one warning was logged; the other features keep working
			}
			GetOverrideName::func = reinterpret_cast<const std::uintptr_t*>(vtable.address())[kGetOverrideNameSlot];
			vtable.write_vfunc(kGetOverrideNameSlot, &GetOverrideName::thunk);
			logger::info("Hook TESLevItem::GetOverrideName installed");
		}
	}
}