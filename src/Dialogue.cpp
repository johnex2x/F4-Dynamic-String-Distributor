#include "Dialogue.h"

#include "AeHookBytes.h"
#include "HookUtil.h"
#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"

namespace
{
	constexpr auto kResponseText = DSD::TypeOf("INFO NAM1");
	constexpr auto kPromptText = DSD::TypeOf("INFO RNAM");

	// Layout from libxse/commonlibf4 RE/D/DialogueResponse.h (not in the alandtse fork)
	struct DialogueResponse
	{
		RE::BSFixedStringCS text;  // 00
		// ... 0x40 total
	};

	// Upstream DSD keys a response by the INFO that owns it. A shared-response INFO (DNAM)
	// points to the owner through dataInfo; the response number is TRDA responseID.
	[[nodiscard]] RE::TESTopicInfo* FindResponseOwner(RE::TESTopicInfo* a_info, const RE::TESResponse* a_response)
	{
		RE::TESTopicInfo* last = a_info;
		std::size_t       depth = 0;
		for (auto info = a_info; info && depth < 16; info = info->dataInfo, ++depth) {
			for (auto response = info->responses.head; response; response = response->pNext) {
				if (response == a_response) {
					return info;
				}
			}
			last = info;
		}
		return last;
	}

	[[nodiscard]] const std::string* FindResponseTranslation(RE::TESTopicInfo* a_info, const RE::TESResponse* a_response)
	{
		const auto owner = FindResponseOwner(a_info, a_response);
		if (!owner) {
			return nullptr;
		}

		const auto  responseID = a_response->data.responseID;
		const auto  entry = Manager::GetSingleton().Store().Find(kResponseText, owner->GetFormID(), responseID);
		const auto& settings = Settings::GetSingleton();
		if (settings.debugLog) {
			logger::trace("Response: \"{}\" INFO {:08X} owner {:08X} responseID {} -> {}",
				static_cast<std::string_view>(a_response->responseText), a_info->GetFormID(), owner->GetFormID(), responseID, entry ? "translated" : "no translation");
		}
		return entry ? &entry->text : nullptr;
	}

	// DialogueResponse::DialogueResponse(topic, info, speaker, response, quest), OG Address Library ID 755245
	// (ID from FallHook src/110163/RuntimeDialogueResponseHook.cpp). Builds the line the game shows and speaks.
	struct DialogueResponseCtor
	{
		static DialogueResponse* thunk(DialogueResponse* a_this, RE::TESTopic* a_topic, RE::TESTopicInfo* a_info, RE::TESObjectREFR* a_speaker, RE::TESResponse* a_response, RE::TESQuest* a_quest)
		{
			auto result = func(a_this, a_topic, a_info, a_speaker, a_response, a_quest);
			if (!result) {
				result = a_this;
			}

			if (result && a_info && a_response) {
				if (const auto translation = FindResponseTranslation(a_info, a_response)) {
					result->text = *translation;
				}
			}
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// No SubtitleManager::ShowSubtitle (875508) fallback: po3 Floating Subtitles hooks that prologue
	// with a fixed-length copy, which relocates our jump wrongly and crashes on the first subtitle.

	// The prompt table (TESTopicInfo* -> RNAM text). Located like XDI does (reg2k/xdi DialogueEx.cpp):
	// pattern "48 83 C6 18 48 83 C7 18", RIP-relative operand of the instruction 0xC bytes later,
	// then -0x28 to the start of the hash table.
	using PromptMap = RE::BSTHashMap<RE::TESTopicInfo*, RE::BSFixedString>;

	// AE: the engine stores the prompt with the case-sensitive string pool variant (BSFixedStringCS::operator=,
	// docs/phaseC-c4-runtimetext-prompts.md risk R2), so the value type is the CS one. Same layout, size 8.
	using PromptMapAE = RE::BSTHashMap<RE::TESTopicInfo*, RE::BSFixedStringCS>;

	[[nodiscard]] PromptMap* FindPromptMap()
	{
		static constexpr std::array<std::uint8_t, 8> pattern{ 0x48, 0x83, 0xC6, 0x18, 0x48, 0x83, 0xC7, 0x18 };

		const auto text = REL::Module::get().segment(REL::Segment::text);
		const auto begin = reinterpret_cast<const std::uint8_t*>(text.address());
		const auto end = begin + text.size();

		const auto match = std::search(begin, end, pattern.begin(), pattern.end());
		if (match == end || match + 0xC + 7 > end) {
			return nullptr;
		}

		const auto     instruction = match + 0xC;
		std::int32_t   displacement = 0;
		std::memcpy(&displacement, instruction + 3, sizeof(displacement));
		const auto operand = reinterpret_cast<std::uintptr_t>(instruction) + 7 + displacement;

		return reinterpret_cast<PromptMap*>(operand - 0x28);
	}

	// AE: the table is a global BSTHashMap, data ID 2662549 (RVA 191/221/240 = 0x2F00408 / 0x2F00458 / 0x2F0B458;
	// ID 2662550 is the same table +8). The OG pattern scan matches a destructor loop there. The data is not code, so
	// the layout is checked instead of bytes: the address lies in .data, the capacity is zero or a power of two, the free
	// and last-free counters fit it, the chain sentinel at +0x18 points into .data (the exe image holds it from the start),
	// and a non-empty table has an aligned entries pointer at +0x28. Run once per apply, never inside a hook.
	[[nodiscard]] PromptMapAE* FindPromptMapAE()
	{
		constexpr std::uint64_t kPromptTableID = 2662549;
		constexpr std::size_t   kTableSize = 0x30;  // up to and including the entries pointer at +0x28

		const REL::Relocation<std::uintptr_t> table{ REL::ID(kPromptTableID) };
		const auto                            data = REL::Module::get().segment(REL::Segment::data);
		const auto                            address = table.address();
		if (address < data.address() || address > data.address() + data.size() || data.size() - (address - data.address()) < kTableSize) {
			logger::error("INFO RNAM: prompt table {:X} is not inside .data", address);
			return nullptr;
		}

		const auto base = reinterpret_cast<const std::byte*>(address);
		const auto u32 = [&](std::ptrdiff_t a_offset) { std::uint32_t v; std::memcpy(&v, base + a_offset, sizeof(v)); return v; };
		const auto u64 = [&](std::ptrdiff_t a_offset) { std::uint64_t v; std::memcpy(&v, base + a_offset, sizeof(v)); return v; };

		const auto capacity = u32(0xC);
		const auto free = u32(0x10);
		const auto good = u32(0x14);
		const auto sentinel = u64(0x18);
		const bool sentinelOk = sentinel >= data.address() && sentinel < data.address() + data.size();
		const bool shapeOk = (capacity == 0 || std::has_single_bit(capacity)) && free <= capacity && (capacity == 0 || good < capacity) && sentinelOk;
		const auto entries = u64(0x28);
		const bool entriesOk = capacity == 0 ? entries == 0 : (entries != 0 && entries % alignof(void*) == 0);
		if (!shapeOk || !entriesOk) {
			logger::error("INFO RNAM: prompt table {:X} has an unexpected layout (capacity {}, free {}, good {}, sentinel {:X}, entries {:X})",
				address, capacity, free, good, sentinel, entries);
			return nullptr;
		}
		return reinterpret_cast<PromptMapAE*>(address);
	}

	[[nodiscard]] const DSD::Entry* FindPrompt(std::uint32_t a_formID)
	{
		// xTranslator exports RNAM with index 1; upstream DSD files use no index
		const auto& store = Manager::GetSingleton().Store();
		const auto  entry = store.Find(kPromptText, a_formID, 1);
		return entry ? entry : store.Find(kPromptText, a_formID, 0);
	}

	template <class Map>
	void TranslatePrompts(Map& a_map)
	{
		std::size_t prompts = 0;
		std::size_t applied = 0;
		for (auto& [info, prompt] : a_map) {
			++prompts;
			if (!info) {
				continue;
			}
			if (const auto entry = FindPrompt(info->GetFormID())) {
				prompt = entry->text;
				++applied;
			}
		}

		logger::info("INFO RNAM: {} prompts in the game table, {} translated", prompts, applied);
	}
}

namespace Dialogue
{
	void Install()
	{
		if (DSD::IsAE()) {
			// AE ID and expected bytes: docs/phaseC-c4-runtimetext-prompts.md section 2c
			HookUtil::InstallPrologueHookChecked<DialogueResponseCtor>(2227131, "DialogueResponse::DialogueResponse", AeHookBytes::kDialogueResponseCtor);
			return;
		}
		HookUtil::InstallPrologueHook<DialogueResponseCtor>(755245, "DialogueResponse::DialogueResponse");
	}

	void ApplyPrompts()
	{
		if (DSD::IsAE()) {
			const auto map = FindPromptMapAE();
			if (!map) {
				logger::error("INFO RNAM: prompt table not found; player dialogue options stay untranslated");
				return;
			}
			TranslatePrompts(*map);
			return;
		}

		const auto map = FindPromptMap();
		if (!map) {
			logger::error("INFO RNAM: prompt table not found; player dialogue options stay untranslated");
			return;
		}
		TranslatePrompts(*map);
	}
}
