#include "core/TranslationStore.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <ranges>

#include <boost/container_hash/hash.hpp>
#include <glaze/json.hpp>

namespace DSD::detail
{
	// One JSON array element. Unknown keys ("status", ...) are ignored.
	// Glaze reflection needs external linkage, so this is not in an anonymous namespace.
	struct ParseData
	{
		std::string                             form_id;
		std::string                             type;
		std::string                             string;
		std::optional<std::string>              original;
		std::optional<std::uint32_t>            index;
		std::optional<std::uint32_t>            rule;    // INNR WNAM only, see TranslationStore::RuleText
		std::optional<std::uint32_t>            choice;  // PERK EPF2 only, see TranslationStore::Find
		std::optional<std::string>              editor_id;
		std::optional<std::vector<std::string>> conditions;
	};
}

namespace
{
	using DSD::detail::ParseData;

	constexpr std::size_t kMaxReportedEntryErrors = 20;
	constexpr std::string_view kOverwriteFolder = "overwrite";

	// INNR WNAM files carry one entry per rule set for the same (form, index); the writer needs all of them in file order.
	constexpr DSD::TypeCode kRuleTextType = DSD::TypeOf("INNR WNAM");
	constexpr DSD::TypeCode kChoiceType = DSD::TypeOf("PERK EPF2");

	[[nodiscard]] constexpr std::uint64_t SlotKey(std::uint32_t a_formID, std::uint32_t a_rule) noexcept
	{
		return (static_cast<std::uint64_t>(a_formID) << 32) | a_rule;
	}

	[[nodiscard]] char ToLower(char a_c) noexcept
	{
		return a_c >= 'A' && a_c <= 'Z' ? static_cast<char>(a_c - 'A' + 'a') : a_c;
	}

	[[nodiscard]] bool IEquals(std::string_view a_lhs, std::string_view a_rhs) noexcept
	{
		return std::ranges::equal(a_lhs, a_rhs, [](char a, char b) { return ToLower(a) == ToLower(b); });
	}

	[[nodiscard]] std::string ToLower(std::string_view a_str)
	{
		std::string result{ a_str };
		std::ranges::transform(result, result.begin(), [](char c) { return ToLower(c); });
		return result;
	}

	[[nodiscard]] bool ReadFile(const std::filesystem::path& a_path, std::string& a_out)
	{
		std::ifstream file{ a_path, std::ios::binary };
		if (!file) {
			return false;
		}
		a_out.assign(std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{});
		if (a_out.starts_with("\xEF\xBB\xBF")) {  // UTF-8 BOM
			a_out.erase(0, 3);
		}
		return true;
	}

	using PluginIndex = boost::unordered_flat_map<std::string, std::size_t>;  // lower-case name -> load order position

	[[nodiscard]] PluginIndex BuildPluginIndex(std::span<const DSD::PluginInfo> a_loadOrder)
	{
		PluginIndex index;
		index.reserve(a_loadOrder.size());
		for (std::size_t i = 0; i < a_loadOrder.size(); ++i) {
			index.try_emplace(ToLower(a_loadOrder[i].name), i);
		}
		return index;
	}

	struct Folder
	{
		std::filesystem::path path;
		std::size_t           priority;  // higher wins
	};

	// Folders that match a loaded plugin (case-insensitive) or "overwrite", highest priority first
	[[nodiscard]] std::vector<Folder> CollectFolders(const std::filesystem::path& a_root, const PluginIndex& a_plugins, std::size_t a_overwritePriority, DSD::LoadStats& a_stats)
	{
		std::vector<Folder> folders;

		std::error_code ec;
		if (!std::filesystem::is_directory(a_root, ec)) {
			return folders;
		}

		for (const auto& dirEntry : std::filesystem::directory_iterator(a_root, ec)) {
			if (!dirEntry.is_directory()) {
				continue;
			}

			const auto name = dirEntry.path().filename().string();
			if (IEquals(name, kOverwriteFolder)) {
				folders.push_back({ dirEntry.path(), a_overwritePriority });
				continue;
			}

			const auto it = a_plugins.find(ToLower(name));
			if (it == a_plugins.end()) {
				a_stats.ignoredFolders.push_back(name);
				continue;
			}

			folders.push_back({ dirEntry.path(), it->second });
		}

		std::ranges::sort(folders, std::greater{}, &Folder::priority);
		return folders;
	}

	// JSON files in a folder, alphabetically last first
	[[nodiscard]] std::vector<std::filesystem::path> CollectFiles(const std::filesystem::path& a_folder)
	{
		std::vector<std::filesystem::path> files;

		std::error_code ec;
		for (const auto& dirEntry : std::filesystem::directory_iterator(a_folder, ec)) {
			if (dirEntry.is_regular_file() && IEquals(dirEntry.path().extension().string(), ".json")) {
				files.push_back(dirEntry.path());
			}
		}

		std::ranges::sort(files, [](const auto& a, const auto& b) { return ToLower(a.filename().string()) > ToLower(b.filename().string()); });
		return files;
	}
}

namespace DSD
{
	std::optional<TypeCode> MakeTypeCode(std::string_view a_type) noexcept
	{
		// "XXXX YYYY": two 4-char signatures separated by one space
		if (a_type.size() != 9 || a_type[4] != ' ') {
			return std::nullopt;
		}

		TypeCode code = 0;
		for (std::size_t i = 0; i < 9; ++i) {
			if (i == 4) {
				continue;
			}
			const auto c = static_cast<unsigned char>(a_type[i]);
			if (c < 0x20 || c > 0x7E) {
				return std::nullopt;
			}
			code = (code << 8) | (c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
		}
		return code;
	}

	std::string TypeCodeToString(TypeCode a_code)
	{
		std::string result(9, ' ');
		for (std::size_t i = 0, shift = 56; i < 9; ++i) {
			if (i == 4) {
				continue;
			}
			result[i] = static_cast<char>((a_code >> shift) & 0xFF);
			shift -= 8;
		}
		return result;
	}

	std::optional<std::uint32_t> ParseRawFormID(std::string_view a_text) noexcept
	{
		// Same rules as upstream Utils::convertToFormID:
		// without "x", an 8-digit ID carries a load-order prefix that is dropped (FE + 3 ESL digits for light plugins)
		const bool hasX = a_text.find_first_of("xX") != std::string_view::npos;
		if (hasX) {
			const auto pos = a_text.find_first_of("xX");
			a_text.remove_prefix(pos + 1);
		} else if (a_text.size() == 8) {
			const bool light = (a_text[0] == 'F' || a_text[0] == 'f') && (a_text[1] == 'E' || a_text[1] == 'e');
			a_text.remove_prefix(light ? 5 : 2);
		}

		if (a_text.empty() || a_text.size() > 8) {
			return std::nullopt;
		}

		std::uint32_t value = 0;
		const auto [ptr, ec] = std::from_chars(a_text.data(), a_text.data() + a_text.size(), value, 16);
		if (ec != std::errc{} || ptr != a_text.data() + a_text.size() || value == 0) {
			return std::nullopt;
		}
		return value;
	}

	std::uint32_t ToRuntimeFormID(const PluginInfo& a_plugin, std::uint32_t a_raw) noexcept
	{
		if (a_plugin.isLight) {
			return (a_raw & 0xFFF) | 0xFE000000 | (a_plugin.smallFileCompileIndex << 12);
		}
		return (a_raw & 0xFFFFFF) | (a_plugin.compileIndex << 24);
	}

	std::string MakeDebugPrefix(std::uint32_t a_runtimeFormID, std::string_view a_plugin)
	{
		return std::format("{:08X}|{}|", a_runtimeFormID, a_plugin);
	}

	std::size_t TranslationStore::KeyHash::operator()(const Key& a_key) const noexcept
	{
		std::size_t seed = 0;
		boost::hash_combine(seed, a_key.type);
		boost::hash_combine(seed, a_key.formID);
		boost::hash_combine(seed, a_key.index);
		boost::hash_combine(seed, a_key.choice);
		return seed;
	}

	void TranslationStore::Clear()
	{
		_entries.clear();
		_keysByType.clear();
		_ruleTexts.clear();
		_ruleTextsAny.clear();
		_ruleSlots.clear();
		_ruleTextCount = 0;
	}

	LoadStats TranslationStore::Load(const std::filesystem::path& a_root, std::span<const PluginInfo> a_loadOrder, bool a_debugInfo)
	{
		Clear();

		LoadStats stats;
		const auto addEntryError = [&](std::string a_message) {
			if (stats.errors.size() < kMaxReportedEntryErrors) {
				stats.errors.push_back(std::move(a_message));
			}
		};

		const auto plugins = BuildPluginIndex(a_loadOrder);

		std::string buffer;
		std::string pluginKey;
		for (const auto& folder : CollectFolders(a_root, plugins, a_loadOrder.size(), stats)) {
			for (const auto& file : CollectFiles(folder.path)) {
				const auto fileName = file.string();

				if (!ReadFile(file, buffer)) {
					++stats.filesFailed;
					stats.errors.push_back(std::format("Cannot read file {}", fileName));
					continue;
				}

				std::vector<ParseData> data;
				if (const auto err = glz::read<glz::opts{ .error_on_unknown_keys = false }>(data, buffer); err) {
					++stats.filesFailed;
					stats.errors.push_back(std::format("Error parsing file {}: {}", fileName, glz::format_error(err, buffer)));
					continue;
				}
				++stats.filesParsed;

				// Later entries in a file win: walk backwards, keep the first stored
				for (auto& item : std::views::reverse(data)) {
					++stats.entriesRead;

					const auto type = MakeTypeCode(item.type);
					if (!type) {
						++stats.entriesBadType;
						addEntryError(std::format("Bad type \"{}\" in {}", item.type, fileName));
						continue;
					}

					const auto separator = item.form_id.find('|');
					const auto raw = separator == std::string::npos ? std::nullopt : ParseRawFormID(std::string_view{ item.form_id }.substr(0, separator));
					if (!raw) {
						++stats.entriesBadFormID;
						addEntryError(std::format("Bad form_id \"{}\" in {}", item.form_id, fileName));
						continue;
					}

					pluginKey = ToLower(std::string_view{ item.form_id }.substr(separator + 1));
					const auto plugin = plugins.find(pluginKey);
					if (plugin == plugins.end()) {
						++stats.entriesPluginNotLoaded;
						continue;
					}

					const auto& pluginInfo = a_loadOrder[plugin->second];
					const auto  choice = *type == kChoiceType ? item.choice.value_or(0) : 0;
					const Key   key{ *type, ToRuntimeFormID(pluginInfo, *raw), item.index.value_or(0), choice };
					auto&     typeStats = stats.types[*type];
					if (a_debugInfo) {
						item.string.insert(0, MakeDebugPrefix(key.formID, pluginInfo.name));
					}
					if (*type == kRuleTextType) {
						if (!item.original) {
							++typeStats.noOriginal;
							continue;
						}
						if (item.rule) {
							// a slot entry is for its position only: it stays out of the text lookups
							const auto slot = SlotKey(key.formID, *item.rule);
							if (_ruleSlots.try_emplace(slot, RuleSlot{ std::move(*item.original), std::move(item.string) }).second) {
								++typeStats.stored;
								++_ruleTextCount;
							} else {
								++typeStats.shadowed;
							}
							continue;
						}
						if (_ruleTexts[key.formID].try_emplace(*item.original, item.string).second) {
							++typeStats.stored;
							++_ruleTextCount;
						} else {
							++typeStats.shadowed;
						}
						_ruleTextsAny.try_emplace(std::move(*item.original), std::move(item.string));
						continue;
					}

					const auto [it, inserted] = _entries.try_emplace(
						key,
						Entry{
							std::move(item.string),
							std::move(item.editor_id),
							std::move(item.original),
							std::move(item.conditions),
							choice ? std::optional{ choice } : std::nullopt });
					if (inserted) {
						++typeStats.stored;
					} else {
						++typeStats.shadowed;
					}
				}
			}
		}

		for (const auto& key : _entries | std::views::keys) {
			_keysByType[key.type].push_back(key);
		}

		return stats;
	}

	const Entry* TranslationStore::Find(TypeCode a_type, std::uint32_t a_formID, std::uint32_t a_index, std::uint32_t a_choice) const
	{
		const auto it = _entries.find(Key{ a_type, a_formID, a_index, a_choice });
		return it != _entries.end() ? std::addressof(it->second) : nullptr;
	}

	const std::string* TranslationStore::RuleText(std::uint32_t a_formID, std::uint32_t a_rule, std::string_view a_text, RuleMatch* a_match) const
	{
		auto match = RuleMatch::kNoSlot;
		if (const auto slot = _ruleSlots.find(SlotKey(a_formID, a_rule)); slot != _ruleSlots.end()) {
			if (slot->second.original == a_text || slot->second.text == a_text) {
				if (a_match) {
					*a_match = RuleMatch::kSlot;
				}
				return std::addressof(slot->second.text);
			}
			match = RuleMatch::kSlotOther;
		}
		if (a_match) {
			*a_match = match;
		}
		const std::string text{ a_text };
		if (const auto form = _ruleTexts.find(a_formID); form != _ruleTexts.end()) {
			if (const auto it = form->second.find(text); it != form->second.end()) {
				return std::addressof(it->second);
			}
		}
		const auto it = _ruleTextsAny.find(text);
		return it != _ruleTextsAny.end() ? std::addressof(it->second) : nullptr;
	}

	void TranslationStore::ForEach(TypeCode a_type, const Visitor& a_visitor) const
	{
		const auto keys = _keysByType.find(a_type);
		if (keys == _keysByType.end()) {
			return;
		}

		for (const auto& key : keys->second) {
			a_visitor(key.formID, key.index, _entries.at(key));
		}
	}

	std::vector<TypeCode> TranslationStore::Types() const
	{
		std::vector<TypeCode> types;
		types.reserve(_keysByType.size());
		for (const auto& type : _keysByType | std::views::keys) {
			types.push_back(type);
		}
		std::ranges::sort(types);
		return types;
	}
}
