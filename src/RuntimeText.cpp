#include "RuntimeText.h"

#include "AeHookBytes.h"
#include "ConstTranslation.h"
#include "HookUtil.h"
#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"
#include "core/QuestLogNumbers.h"

namespace
{
	constexpr auto kJournalText = DSD::TypeOf("QUST CNAM");

	using DSD::IsDescriptionType;

	struct Tables
	{
		// Each form owns its TESDescription component, so the component address identifies the form
		boost::unordered_flat_map<const RE::TESDescription*, const std::string*> descriptions;
		// Runtime quest form ID -> journal numbering from the quest's own plugin
		boost::unordered_flat_map<std::uint32_t, DSD::QuestLogNumbers> questLogs;
	};

	// Replaced as a whole, so hooks on other threads never see a half-built table
	std::atomic<std::shared_ptr<const Tables>> g_tables;

	// BSStringT<char>::Set(const char*, maxLen): the game's own string setter. OG: the fork's wrapper (its ID is the
	// OG one). AE: engine function ID 4471693, whose code is checked in Install; the fork's ID (2189084) is absent
	// there. Chosen once at Install, before either hook exists, so the hooks carry no runtime check.
	using StringSet = void(RE::BSStringT<char>*, const char*, std::uint64_t);

	void SetViaFork(RE::BSStringT<char>* a_out, const char* a_text, std::uint64_t a_maxLen)
	{
		static_cast<void>(a_out->Set(a_text, a_maxLen));
	}

	StringSet* g_stringSet = SetViaFork;

	void BuildDescriptions(Tables& a_tables)
	{
		const auto& store = Manager::GetSingleton().Store();
		std::size_t entries = 0;
		std::size_t missing = 0;
		std::size_t noDescription = 0;
		for (const auto type : store.Types()) {
			if (!IsDescriptionType(type)) {
				continue;
			}
			store.ForEach(type, [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry& a_entry) {
				++entries;
				const auto form = DSD::LookupFormByID(a_formID);
				if (!form) {
					++missing;
					return;
				}
				const auto description = form->As<RE::TESDescription>();
				if (!description) {
					++noDescription;
					logger::debug("{} {:08X}: form type {} has no description", DSD::TypeCodeToString(type), a_formID, form->GetFormTypeString());
					return;
				}
				a_tables.descriptions.try_emplace(description, &a_entry.text);
			});
		}
		logger::info("XXXX DESC: {} entries, {} descriptions ready, {} form missing, {} without description", entries, a_tables.descriptions.size(), missing, noDescription);
	}

	void BuildQuestLogs(Tables& a_tables)
	{
		const auto& store = Manager::GetSingleton().Store();

		// Group the translated quests by the plugin that defines them
		std::map<RE::TESFile*, std::vector<std::uint32_t>> questsByFile;
		std::size_t                                         missing = 0;
		store.ForEach(kJournalText, [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry&) {
			const auto form = DSD::LookupFormByID(a_formID);
			const auto quest = form ? form->As<RE::TESQuest>() : nullptr;
			const auto file = quest ? quest->GetFile(0) : nullptr;
			if (!file) {
				++missing;
				return;
			}
			auto& quests = questsByFile[file];
			if (std::ranges::find(quests, a_formID) == quests.end()) {
				quests.push_back(a_formID);
			}
		});

		for (const auto& [file, quests] : questsByFile) {
			const auto    path = std::filesystem::path{ "Data" } / file->GetFilename();
			std::ifstream stream{ path, std::ios::binary };
			if (!stream) {
				logger::error("QUST CNAM: cannot open {}", path.string());
				continue;
			}

			std::vector<std::uint32_t> lowIDs;
			for (const auto formID : quests) {
				lowIDs.push_back(DSD::ToFileLocalFormID(formID));
			}
			try {
				auto index = DSD::ReadQuestLogNumbers(stream, lowIDs);
				for (const auto formID : quests) {
					if (const auto it = index.find(DSD::ToFileLocalFormID(formID)); it != index.end()) {
						a_tables.questLogs.insert_or_assign(formID, std::move(it->second));
					}
				}
			} catch (const std::exception& e) {
				logger::error("QUST CNAM: cannot read {}: {}", path.string(), e.what());
			}
		}

		logger::info("QUST CNAM: {} quests with journal translations, {} numbered from their plugin files, {} form missing",
			std::accumulate(questsByFile.begin(), questsByFile.end(), std::size_t{ 0 }, [](std::size_t a_sum, const auto& a_pair) { return a_sum + a_pair.second.size(); }),
			a_tables.questLogs.size(), missing);
	}

	// TESDescription::GetDescription(BSString& out, const TESForm* parent), OG Address Library ID 523613
	// (CommonLibF4 FormComponents.h). The game reads the description from the plugin on every call.
	struct GetDescription
	{
		static void thunk(RE::TESDescription* a_this, RE::BSStringT<char>& a_out, const RE::TESForm* a_parent)
		{
			func(a_this, a_out, a_parent);

			const auto tables = g_tables.load(std::memory_order_acquire);
			if (!tables || !a_this) {
				return;
			}
			if (const auto it = tables->descriptions.find(a_this); it != tables->descriptions.end()) {
				g_stringSet(&a_out, it->second->c_str(), it->second->size() + 1);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// TESQuest::GetJournalTextForStageItem(BSString& out, stage, item), OG Address Library ID 139753
	// (ID and signature from FallHook src/110163/RuntimeQuestJournalTextHook.cpp)
	struct GetJournalText
	{
		static void thunk(RE::TESQuest* a_quest, RE::BSStringT<char>& a_out, std::uint16_t a_stage, std::uint8_t a_item)
		{
			func(a_quest, a_out, a_stage, a_item);

			const auto tables = g_tables.load(std::memory_order_acquire);
			if (!tables || !a_quest) {
				return;
			}
			const auto formID = a_quest->GetFormID();
			const auto log = tables->questLogs.find(formID);
			if (log == tables->questLogs.end()) {
				return;
			}

			const auto number = log->second.Find(a_stage, a_item);
			const auto entry = number ? Manager::GetSingleton().Store().Find(kJournalText, formID, *number) : nullptr;
			if (Settings::GetSingleton().debugLog) {
				logger::trace("Journal: quest {:08X} stage {} item {} -> number {} -> {}", formID, a_stage, a_item, number.value_or(0), entry ? "translated" : "no translation");
			}
			if (entry) {
				g_stringSet(&a_out, entry->text.c_str(), entry->text.size() + 1);
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};
}

namespace RuntimeText
{
	void Install()
	{
		if (DSD::IsAE()) {
			// AE IDs and expected bytes: docs/phaseC-c4-runtimetext-prompts.md. Both hooks write through
			// BSStringT::Set, so its code is checked first; each hook is then checked on its own.
			const REL::Relocation<std::uintptr_t> set{ REL::ID(4471693) };
			if (!HookUtil::CodeMatches(set.address(), AeHookBytes::kBSStringSet, "BSStringT::Set")) {
				return;
			}
			g_stringSet = reinterpret_cast<StringSet*>(set.address());
			HookUtil::InstallPrologueHookChecked<GetDescription>(2193019, "TESDescription::GetDescription", AeHookBytes::kGetDescription);
			HookUtil::InstallPrologueHookChecked<GetJournalText>(2207825, "TESQuest::GetJournalTextForStageItem", AeHookBytes::kGetJournalText);
			return;
		}
		HookUtil::InstallPrologueHook<GetDescription>(523613, "TESDescription::GetDescription");
		HookUtil::InstallPrologueHook<GetJournalText>(139753, "TESQuest::GetJournalTextForStageItem");
	}

	void Apply()
	{
		auto tables = std::make_shared<Tables>();
		BuildDescriptions(*tables);
		BuildQuestLogs(*tables);
		g_tables.store(std::move(tables), std::memory_order_release);
	}

	void Reset()
	{
		g_tables.store(nullptr, std::memory_order_release);
	}
}
