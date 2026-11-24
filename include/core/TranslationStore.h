#pragma once

// Game-independent translation store: reads the DSD folder tree and answers lookups.
// It never touches game objects, so it can be tested without Fallout 4 running.

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

namespace DSD
{
	// "INFO NAM1" packed into 8 bytes (record signature + subrecord signature)
	using TypeCode = std::uint64_t;

	[[nodiscard]] std::optional<TypeCode> MakeTypeCode(std::string_view a_type) noexcept;
	[[nodiscard]] std::string             TypeCodeToString(TypeCode a_code);

	// Compile-time variant for well-formed literals, e.g. TypeOf("INFO NAM1")
	[[nodiscard]] consteval TypeCode TypeOf(const char (&a_type)[10])
	{
		TypeCode code = 0;
		for (std::size_t i = 0; i < 9; ++i) {
			if (i == 4) {
				continue;
			}
			const auto c = static_cast<unsigned char>(a_type[i]);
			code = (code << 8) | (c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
		}
		return code;
	}

	struct PluginInfo
	{
		std::string   name;  // e.g. "Fallout4.esm"
		std::uint32_t compileIndex{ 0 };
		std::uint32_t smallFileCompileIndex{ 0 };
		bool          isLight{ false };
	};

	// Raw form ID as written in the JSON ("0024A003", "0x24A003", "24A003", "FE000ABC"), without load-order bits
	[[nodiscard]] std::optional<std::uint32_t> ParseRawFormID(std::string_view a_text) noexcept;
	[[nodiscard]] std::uint32_t                ToRuntimeFormID(const PluginInfo& a_plugin, std::uint32_t a_raw) noexcept;
	// The form ID a plugin file uses for its own record, without the master-index byte, so it can be compared
	// with a file record ID & 0x00FFFFFF. Light plugins (runtime FExxxyyy) keep only the 3 ESL digits.
	[[nodiscard]] constexpr std::uint32_t      ToFileLocalFormID(std::uint32_t a_runtimeFormID) noexcept
	{
		return (a_runtimeFormID >> 24) == 0xFE ? (a_runtimeFormID & 0xFFF) : (a_runtimeFormID & 0x00FFFFFF);
	}

	// "0024A003|Fallout4.esm|": shown in front of a text when bEnableDebugInfo is on, so translators see where it came from.
	// a_runtimeFormID includes the load-order bits (same value the game shows for the form).
	[[nodiscard]] std::string MakeDebugPrefix(std::uint32_t a_runtimeFormID, std::string_view a_plugin);

	struct Entry
	{
		std::string                             text;
		std::optional<std::string>              editorID;
		std::optional<std::string>              original;
		std::optional<std::vector<std::string>> conditions;
		std::optional<std::uint32_t>            choice;  // PERK EPF2 only, see TranslationStore::Find
	};

	struct LoadStats
	{
		struct TypeStats
		{
			std::size_t stored{ 0 };
			std::size_t shadowed{ 0 };  // same key already provided by a higher-priority entry
			std::size_t noOriginal{ 0 };  // INNR WNAM: entries without "original", which cannot be matched
		};

		std::size_t filesParsed{ 0 };
		std::size_t filesFailed{ 0 };
		std::size_t entriesRead{ 0 };
		std::size_t entriesBadFormID{ 0 };
		std::size_t entriesBadType{ 0 };
		std::size_t entriesPluginNotLoaded{ 0 };
		std::vector<std::string> ignoredFolders;  // folder not in load order
		std::vector<std::string> errors;          // file parse errors and the first bad entries
		boost::unordered_flat_map<TypeCode, TypeStats> types;
	};

	class TranslationStore
	{
	public:
		// Priority: "overwrite" folder first, then later plugins in load order,
		// then files later in alphabetical order, then later entries in a file.
		// The first entry stored for a key wins (same rule as upstream DSD).
		//
		// a_debugInfo: put MakeDebugPrefix() in front of every stored text. Doing it here, in the one place
		// all writers read from, means every writer shows the prefix without its own code.
		LoadStats Load(const std::filesystem::path& a_root, std::span<const PluginInfo> a_loadOrder, bool a_debugInfo = false);
		void      Clear();

		// a_index: 0 when the JSON entry has no index
		// a_choice: PERK EPF2 only. An entry with "choice" (1-based position among the perk's Activate choices) is
		// kept apart from the entries with the same index: a mod can give several choices of one perk the same
		// EPF3 index (design decision 2026-10-22). 0 = no choice.
		[[nodiscard]] const Entry* Find(TypeCode a_type, std::uint32_t a_formID, std::uint32_t a_index = 0, std::uint32_t a_choice = 0) const;

		using Visitor = std::function<void(std::uint32_t a_formID, std::uint32_t a_index, const Entry& a_entry)>;
		void ForEach(TypeCode a_type, const Visitor& a_visitor) const;

		// INNR WNAM is matched by the rule's current text, not by index: mods override the vanilla INNR records,
		// so an index from one plugin's version does not fit the version the game uses (design decision 2026-10-21).
		// Looks in the entries for this form first, then in those of every form. a_text: the rule's text now.
		//
		// An entry with "rule" (1-based position among all the record's rules, rule set by rule set) wins at that
		// position when its original is the rule's text now: the Korean patch gave one record's rules with different
		// English names (Steady, Recoil Compensated) the same Korean text (design decision 2026-10-22).
		// a_rule: the rule's position in the record the game uses.
		// a_match: how the text was found, for the log. A rule that already holds its slot entry's text counts as kSlot.
		enum class RuleMatch
		{
			kNoSlot,     // no entry with "rule" at this position: text lookup only
			kSlot,       // the entry with "rule" at this position
			kSlotOther,  // an entry with "rule" at this position, but its original is not the rule's text: text lookup
		};
		[[nodiscard]] const std::string* RuleText(std::uint32_t a_formID, std::uint32_t a_rule, std::string_view a_text, RuleMatch* a_match = nullptr) const;
		[[nodiscard]] bool               HasRuleTexts() const noexcept { return !_ruleTextsAny.empty() || !_ruleSlots.empty(); }

		[[nodiscard]] std::vector<TypeCode> Types() const;  // sorted
		[[nodiscard]] std::size_t           Size() const noexcept { return _entries.size() + _ruleTextCount; }

	private:
		struct Key
		{
			TypeCode      type;
			std::uint32_t formID;
			std::uint32_t index;
			std::uint32_t choice;  // 0 = no choice

			bool operator==(const Key&) const = default;
		};

		struct KeyHash
		{
			std::size_t operator()(const Key& a_key) const noexcept;
		};

		boost::unordered_flat_map<Key, Entry, KeyHash>          _entries;
		boost::unordered_flat_map<TypeCode, std::vector<Key>> _keysByType;  // built after Load

		using TextMap = boost::unordered_flat_map<std::string, std::string>;  // original -> translation
		boost::unordered_flat_map<std::uint32_t, TextMap> _ruleTexts;    // INNR form -> its entries
		TextMap                                            _ruleTextsAny;  // entries of every INNR form

		struct RuleSlot
		{
			std::string original;
			std::string text;
		};
		// (INNR form << 32 | rule position) -> entry
		boost::unordered_flat_map<std::uint64_t, RuleSlot> _ruleSlots;
		std::size_t                                        _ruleTextCount{ 0 };
	};
}
