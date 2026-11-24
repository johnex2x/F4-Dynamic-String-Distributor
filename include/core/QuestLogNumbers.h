#pragma once

// QUST CNAM (quest journal entry) numbering as ESP-ESM Translator exports it.
// The JSON index counts only the log entries (QSDT) whose CNAM has text, across all stages of the quest in file
// order, starting at 1: an entry without CNAM, or with an empty or whitespace-only one, gets no number and does not
// advance the count. (A localized plugin stores a string ID instead; ID 0 means no text.)
// The game identifies a journal entry by (stage index, entry position inside the stage) instead,
// so the plugin file is read once to translate between the two. No game objects are touched.

#include <cstdint>
#include <istream>
#include <optional>
#include <span>

#include <boost/unordered/unordered_flat_map.hpp>

namespace DSD
{
	class QuestLogNumbers
	{
	public:
		// 1-based journal number, or nullopt when the stage or entry is not in the record or has no text
		[[nodiscard]] std::optional<std::uint32_t> Find(std::uint16_t a_stage, std::uint8_t a_item) const;
		[[nodiscard]] std::size_t                  Size() const noexcept { return _numbers.size(); }

		void Add(std::uint16_t a_stage, std::uint8_t a_item, std::uint32_t a_number) { _numbers.try_emplace(Key(a_stage, a_item), a_number); }

	private:
		[[nodiscard]] static constexpr std::uint32_t Key(std::uint16_t a_stage, std::uint8_t a_item) noexcept { return (static_cast<std::uint32_t>(a_stage) << 8) | a_item; }

		boost::unordered_flat_map<std::uint32_t, std::uint32_t> _numbers;
	};

	// Quest form ID with the plugin's own load-order byte cleared (the low 24 bits) -> numbering
	using QuestLogIndex = boost::unordered_flat_map<std::uint32_t, QuestLogNumbers>;

	// Reads the QUST records that the plugin itself defines (not overrides of its masters).
	// a_quests: low 24 bits of the quests to read; empty reads every quest.
	// Throws std::runtime_error on a malformed file.
	[[nodiscard]] QuestLogIndex ReadQuestLogNumbers(std::istream& a_plugin, std::span<const std::uint32_t> a_quests = {});
}
