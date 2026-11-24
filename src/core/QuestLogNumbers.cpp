#include "core/QuestLogNumbers.h"

#include "PluginFile.h"

using namespace DSD::PluginFile;

namespace
{
	// Localized plugin: CNAM is a string ID, 0 means no text (the string table is not available here).
	// Otherwise CNAM is a null-terminated string that needs a non-whitespace character.
	[[nodiscard]] bool HasText(const std::uint8_t* a_sub, std::size_t a_size, bool a_localized)
	{
		if (a_localized) {
			return a_size >= 4 && Read<std::uint32_t>(a_sub) != 0;
		}
		for (std::size_t i = 0; i < a_size && a_sub[i]; ++i) {
			if (a_sub[i] != ' ' && a_sub[i] != '\t' && a_sub[i] != '\r' && a_sub[i] != '\n') {
				return true;
			}
		}
		return false;
	}

	// Log entries start with QSDT and belong to the stage of the last INDX. Only entries whose CNAM has text get
	// a journal number (the ESP-ESM Translator rule). Known gap: a localized CNAM whose string is just " " (vanilla
	// BoS100 0x05DDAB) has a non-zero ID, so the later entries of that quest are off by one.
	[[nodiscard]] DSD::QuestLogNumbers ParseQuest(const std::vector<std::uint8_t>& a_data, bool a_localized)
	{
		DSD::QuestLogNumbers         numbers;
		std::optional<std::uint16_t> stage;
		std::uint32_t                itemInStage = 0;  // entries started in this stage
		std::uint32_t                number = 0;
		bool                         awaitingCnam = false;
		ForEachSubrecord(a_data, [&](std::string_view a_sig, const std::uint8_t* a_sub, std::size_t a_size) {
			if (a_sig == "INDX" && a_size >= 2) {
				stage = Read<std::uint16_t>(a_sub);
				itemInStage = 0;
				awaitingCnam = false;
			} else if (a_sig == "QSDT" && stage) {
				++itemInStage;
				awaitingCnam = true;
			} else if (a_sig == "CNAM" && stage && awaitingCnam) {
				awaitingCnam = false;  // only the first CNAM after a QSDT counts
				if (HasText(a_sub, a_size, a_localized) && itemInStage <= 0x100) {
					numbers.Add(*stage, static_cast<std::uint8_t>(itemInStage - 1), ++number);
				}
			} else if (a_sig == "QOBJ") {
				stage.reset();  // objectives follow the stages
			}
		});
		return numbers;
	}
}

namespace DSD
{
	std::optional<std::uint32_t> QuestLogNumbers::Find(std::uint16_t a_stage, std::uint8_t a_item) const
	{
		const auto it = _numbers.find(Key(a_stage, a_item));
		return it != _numbers.end() ? std::optional{ it->second } : std::nullopt;
	}

	QuestLogIndex ReadQuestLogNumbers(std::istream& a_plugin, std::span<const std::uint32_t> a_quests)
	{
		QuestLogIndex index;
		bool          localized = false;
		ForEachOwnRecord(a_plugin, "QUST", a_quests, [&](std::uint32_t a_low, const std::vector<std::uint8_t>& a_data) {
			index.insert_or_assign(a_low, ParseQuest(a_data, localized));
		}, &localized);
		return index;
	}
}
