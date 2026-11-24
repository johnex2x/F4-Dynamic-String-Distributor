#include "core/TranslationStore.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <format>
#include <fstream>

using namespace DSD;

namespace
{
	// Temporary DSD folder tree, removed at scope exit
	class TempTree
	{
	public:
		TempTree()
		{
			static std::atomic<int> counter{ 0 };
			_root = std::filesystem::temp_directory_path() / std::format("dsd-tests-{}-{}", std::chrono::steady_clock::now().time_since_epoch().count(), counter++);
			std::filesystem::remove_all(_root);
			std::filesystem::create_directories(_root);
		}
		~TempTree() { std::filesystem::remove_all(_root); }

		void Write(const std::filesystem::path& a_relative, std::string_view a_content) const
		{
			const auto path = _root / a_relative;
			std::filesystem::create_directories(path.parent_path());
			std::ofstream{ path, std::ios::binary } << a_content;
		}

		[[nodiscard]] const std::filesystem::path& Root() const noexcept { return _root; }

	private:
		std::filesystem::path _root;
	};

	std::string Item(std::string_view a_formID, std::string_view a_type, std::string_view a_text, std::optional<std::uint32_t> a_index = std::nullopt, std::string_view a_extra = {})
	{
		auto result = std::format(R"({{"form_id":"{}","type":"{}","string":"{}")", a_formID, a_type, a_text);
		if (a_index) {
			result += std::format(R"(,"index":{})", *a_index);
		}
		if (!a_extra.empty()) {
			result += ',';
			result += a_extra;
		}
		return result + '}';
	}

	std::string Array(std::initializer_list<std::string> a_items)
	{
		std::string result = "[";
		for (const auto& item : a_items) {
			if (result.size() > 1) {
				result += ',';
			}
			result += item;
		}
		return result + ']';
	}

	const std::vector<PluginInfo> kLoadOrder{
		{ "Fallout4.esm", 0x00, 0, false },
		{ "DLCRobot.esm", 0x01, 0, false },
		{ "MyLight.esl", 0xFE, 0x002, true },
		{ "Patch.esp", 0x02, 0, false },
	};

	const std::string* Text(const TranslationStore& a_store, TypeCode a_type, std::uint32_t a_formID, std::uint32_t a_index = 0)
	{
		const auto entry = a_store.Find(a_type, a_formID, a_index);
		return entry ? &entry->text : nullptr;
	}
}

TEST_CASE("type codes", "[store]")
{
	CHECK(MakeTypeCode("INFO NAM1") == TypeOf("INFO NAM1"));
	CHECK(MakeTypeCode("npc_ full") == TypeOf("NPC_ FULL"));
	CHECK(TypeCodeToString(TypeOf("GMST DATA")) == "GMST DATA");
	CHECK_FALSE(MakeTypeCode("INFONAM1").has_value());
	CHECK_FALSE(MakeTypeCode("INFO_NAM1").has_value());
	CHECK_FALSE(MakeTypeCode("").has_value());
}

TEST_CASE("form id spellings", "[store]")
{
	CHECK(ParseRawFormID("0024A003") == 0x24A003u);  // load-order prefix dropped
	CHECK(ParseRawFormID("050176CD") == 0x0176CDu);
	CHECK(ParseRawFormID("0x0176CD") == 0x0176CDu);
	CHECK(ParseRawFormID("176CD") == 0x176CDu);
	CHECK(ParseRawFormID("0x6CD") == 0x6CDu);
	CHECK(ParseRawFormID("FE0016CD") == 0x6CDu);  // light plugin: keep only the 3 ESL digits
	CHECK_FALSE(ParseRawFormID("").has_value());
	CHECK_FALSE(ParseRawFormID("zz").has_value());
	CHECK_FALSE(ParseRawFormID("00000000").has_value());
}

TEST_CASE("runtime form ids", "[store]")
{
	CHECK(ToRuntimeFormID({ "Patch.esp", 0x02, 0, false }, 0x24A003) == 0x0224A003u);
	CHECK(ToRuntimeFormID({ "Patch.esp", 0x02, 0, false }, 0x0124A003) == 0x0224A003u);
	CHECK(ToRuntimeFormID({ "MyLight.esl", 0xFE, 0x002, true }, 0x8AB) == 0xFE0028ABu);
	CHECK(ToRuntimeFormID({ "MyLight.esl", 0xFE, 0x002, true }, 0xFE0018AB) == 0xFE0028ABu);
}

TEST_CASE("file-local form ids match the record IDs in the plugin file", "[store]")
{
	CHECK(ToFileLocalFormID(0x0224A003) == 0x24A003u);
	// ESL-flagged plugin at light slot 0x0A5: its record 02000829 in the file is 0x000829 after masking
	CHECK(ToFileLocalFormID(0xFE0A5829) == (0x02000829u & 0x00FFFFFF));
}

TEST_CASE("loads entries and maps them to runtime form ids", "[store]")
{
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({
		Item("0024A003|Fallout4.esm", "MISC FULL", "螺絲"),
		Item("00000801|MyLight.esl", "WEAP FULL", "輕量"),
		Item("00001234|Patch.esp", "ARMO FULL", "補丁"),
	}));

	TranslationStore store;
	const auto stats = store.Load(tree.Root(), kLoadOrder);

	CHECK(stats.filesParsed == 1);
	CHECK(stats.entriesRead == 3);
	CHECK(store.Size() == 3);
	REQUIRE(Text(store, TypeOf("MISC FULL"), 0x0024A003));
	CHECK(*Text(store, TypeOf("MISC FULL"), 0x0024A003) == "螺絲");
	CHECK(Text(store, TypeOf("WEAP FULL"), 0xFE002801));
	CHECK(Text(store, TypeOf("ARMO FULL"), 0x02001234));
}

TEST_CASE("folder names match plugins case-insensitively; unknown folders are ignored", "[store]")
{
	TempTree tree;
	tree.Write("fallout4.ESM/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "A") }));
	tree.Write("NotLoaded.esp/a.json", Array({ Item("00000002|Fallout4.esm", "MISC FULL", "B") }));

	TranslationStore store;
	const auto stats = store.Load(tree.Root(), kLoadOrder);

	CHECK(Text(store, TypeOf("MISC FULL"), 0x00000001));
	CHECK_FALSE(Text(store, TypeOf("MISC FULL"), 0x00000002));
	REQUIRE(stats.ignoredFolders.size() == 1);
	CHECK(stats.ignoredFolders[0] == "NotLoaded.esp");
}

TEST_CASE("priority: overwrite > later plugin folder > later file name > later entry", "[store]")
{
	const auto misc = TypeOf("MISC FULL");

	SECTION("overwrite folder wins over plugin folders")
	{
		TempTree tree;
		tree.Write("Patch.esp/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "patch") }));
		tree.Write("overwrite/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "overwrite") }));
		TranslationStore store;
		const auto stats = store.Load(tree.Root(), kLoadOrder);
		CHECK(*Text(store, misc, 0x00000001) == "overwrite");
		CHECK(stats.types.at(misc).shadowed == 1);
	}

	SECTION("later plugin in load order wins")
	{
		TempTree tree;
		tree.Write("Fallout4.esm/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "base") }));
		tree.Write("Patch.esp/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "patch") }));
		TranslationStore store;
		store.Load(tree.Root(), kLoadOrder);
		CHECK(*Text(store, misc, 0x00000001) == "patch");
	}

	SECTION("alphabetically later file wins within a folder")
	{
		TempTree tree;
		tree.Write("Fallout4.esm/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "a") }));
		tree.Write("Fallout4.esm/B.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "b") }));
		TranslationStore store;
		store.Load(tree.Root(), kLoadOrder);
		CHECK(*Text(store, misc, 0x00000001) == "b");
	}

	SECTION("later entry wins within a file")
	{
		TempTree tree;
		tree.Write("Fallout4.esm/a.json", Array({
			Item("00000001|Fallout4.esm", "MISC FULL", "first"),
			Item("00000001|Fallout4.esm", "MISC FULL", "second"),
		}));
		TranslationStore store;
		const auto stats = store.Load(tree.Root(), kLoadOrder);
		CHECK(*Text(store, misc, 0x00000001) == "second");
		CHECK(stats.types.at(misc).stored == 1);
		CHECK(stats.types.at(misc).shadowed == 1);
	}
}

TEST_CASE("INNR WNAM is matched by the rule's text, this form first", "[store]")
{
	const auto innr = TypeOf("INNR WNAM");
	const auto orig = [](std::string_view a_text) { return std::format(R"("original":"{}")", a_text); };

	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({
		Item("00000001|Fallout4.esm", "INNR WNAM", "base", 1, orig("Fiery")),
		Item("00000001|Fallout4.esm", "INNR WNAM", "aim", 2, orig("Targeting")),
		Item("00000002|Fallout4.esm", "INNR WNAM", "recognize", 1, orig("Targeting")),
		Item("00000002|Fallout4.esm", "INNR WNAM", "no original", 2),
	}));
	tree.Write("Patch.esp/a.json", Array({ Item("00000001|Fallout4.esm", "INNR WNAM", "patch", 7, orig("Fiery")) }));

	TranslationStore store;
	const auto stats = store.Load(tree.Root(), kLoadOrder);

	// The index plays no part; the later plugin wins for the same text
	CHECK(*store.RuleText(0x00000001, 1, "Fiery") == "patch");
	// The same text can have another translation in another form
	CHECK(*store.RuleText(0x00000001, 1, "Targeting") == "aim");
	CHECK(*store.RuleText(0x00000002, 1, "Targeting") == "recognize");
	// A form without its own entry for the text (an INNR a mod added or merged) uses any form's entry
	CHECK(*store.RuleText(0x00000003, 1, "Fiery") == "patch");
	CHECK(store.RuleText(0x00000001, 1, "Pistol") == nullptr);

	CHECK(store.Find(innr, 0x00000001, 1) == nullptr);
	CHECK(stats.types.at(innr).stored == 3);
	CHECK(stats.types.at(innr).shadowed == 1);
	CHECK(stats.types.at(innr).noOriginal == 1);
	CHECK(store.HasRuleTexts());
}

TEST_CASE("INNR WNAM with a rule position wins there while the text still matches", "[store]")
{
	const auto innr = TypeOf("INNR WNAM");
	const auto slot = [](std::string_view a_text, std::uint32_t a_rule) { return std::format(R"("original":"{}","rule":{})", a_text, a_rule); };

	// One record, two rules with the same Korean text (Steady, Recoil Compensated)
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({
		Item("00000001|Fallout4.esm", "INNR WNAM", "穩定", 1, R"("original":"저반동")"),
		Item("00000001|Fallout4.esm", "INNR WNAM", "穩定", 1, slot("저반동", 4)),
		Item("00000001|Fallout4.esm", "INNR WNAM", "後座力補償", 1, slot("저반동", 9)),
	}));

	TranslationStore store;
	const auto stats = store.Load(tree.Root(), kLoadOrder);

	CHECK(*store.RuleText(0x00000001, 4, "저반동") == "穩定");
	CHECK(*store.RuleText(0x00000001, 9, "저반동") == "後座力補償");
	// Another text at that position (a mod moved the rules): the text entries decide
	CHECK(store.RuleText(0x00000001, 9, "고속") == nullptr);
	CHECK(*store.RuleText(0x00000001, 5, "저반동") == "穩定");
	// Slot entries stay out of the text lookups of other forms
	CHECK(*store.RuleText(0x00000002, 9, "저반동") == "穩定");
	CHECK(stats.types.at(innr).stored == 3);

	// How it was found, for the log; a rule that already holds the slot's text (applied before) still counts as the slot
	using Match = TranslationStore::RuleMatch;
	auto match = Match::kNoSlot;
	CHECK(*store.RuleText(0x00000001, 9, "後座力補償", &match) == "後座力補償");
	CHECK(match == Match::kSlot);
	CHECK(store.RuleText(0x00000001, 9, "고속", &match) == nullptr);
	CHECK(match == Match::kSlotOther);
	CHECK(*store.RuleText(0x00000001, 5, "저반동", &match) == "穩定");
	CHECK(match == Match::kNoSlot);
}

TEST_CASE("index is part of the key", "[store]")
{
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({
		Item("00000010|Fallout4.esm", "INFO NAM1", "line one", 1),
		Item("00000010|Fallout4.esm", "INFO NAM1", "line two", 2),
		Item("00000020|Fallout4.esm", "ARMO FULL", "base name", 1),
		Item("00000020|Fallout4.esm", "ARMO FULL", "template name", 3),
		Item("00000030|Fallout4.esm", "DIAL FULL", "no index"),
	}));

	TranslationStore store;
	store.Load(tree.Root(), kLoadOrder);

	CHECK(*Text(store, TypeOf("INFO NAM1"), 0x10, 1) == "line one");
	CHECK(*Text(store, TypeOf("INFO NAM1"), 0x10, 2) == "line two");
	CHECK(*Text(store, TypeOf("ARMO FULL"), 0x20, 1) == "base name");
	CHECK(*Text(store, TypeOf("ARMO FULL"), 0x20, 3) == "template name");
	CHECK(*Text(store, TypeOf("DIAL FULL"), 0x30, 0) == "no index");
	CHECK_FALSE(Text(store, TypeOf("INFO NAM1"), 0x10, 3));

	std::vector<std::uint32_t> armoIndexes;
	store.ForEach(TypeOf("ARMO FULL"), [&](std::uint32_t, std::uint32_t a_index, const Entry&) { armoIndexes.push_back(a_index); });
	std::ranges::sort(armoIndexes);
	CHECK(armoIndexes == std::vector<std::uint32_t>{ 1, 3 });
}

TEST_CASE("PERK EPF2 choices with the same index are kept apart by choice", "[store]")
{
	const auto perk = TypeOf("PERK EPF2");

	// One perk, three Activate choices, all with EPF3 index 0 (CAP_Perk_BindCaptives)
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({
		Item("00000040|Fallout4.esm", "PERK EPF2", "鬆綁俘虜", 0, R"("choice":1)"),
		Item("00000040|Fallout4.esm", "PERK EPF2", "綁住俘虜", 0, R"("choice":2)"),
		Item("00000040|Fallout4.esm", "PERK EPF2", "威脅俘虜", 0, R"("choice":3)"),
		Item("00000041|Fallout4.esm", "PERK EPF2", "no choice", 2),
		Item("00000042|Fallout4.esm", "INFO NAM1", "choice is PERK EPF2 only", 1, R"("choice":2)"),
	}));

	TranslationStore store;
	const auto stats = store.Load(tree.Root(), kLoadOrder);

	CHECK(stats.types.at(perk).stored == 4);
	CHECK(store.Find(perk, 0x40, 0, 2)->text == "綁住俘虜");
	CHECK(store.Find(perk, 0x40, 0, 3)->choice == 3u);
	CHECK_FALSE(store.Find(perk, 0x40, 0));
	CHECK(store.Find(perk, 0x41, 2)->choice == std::nullopt);
	CHECK(store.Find(TypeOf("INFO NAM1"), 0x42, 1)->text == "choice is PERK EPF2 only");

	std::vector<std::string> texts;
	store.ForEach(perk, [&](std::uint32_t a_formID, std::uint32_t, const Entry& a_entry) {
		if (a_formID == 0x40) {
			texts.push_back(a_entry.text);
		}
	});
	std::ranges::sort(texts);
	CHECK(texts == std::vector<std::string>{ "威脅俘虜", "綁住俘虜", "鬆綁俘虜" });
}

TEST_CASE("optional fields are kept; unknown fields are ignored", "[store]")
{
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({
		Item("0024A002|Fallout4.esm", "GMST DATA", "輸入", 1, R"("editor_id":"sIn","original":"In","status":"TranslationComplete")"),
		Item("00000040|Fallout4.esm", "NPC_ FULL", "守衛", std::nullopt, R"("conditions":["IsInCombat == 1"])"),
	}));

	TranslationStore store;
	store.Load(tree.Root(), kLoadOrder);

	const auto gmst = store.Find(TypeOf("GMST DATA"), 0x0024A002, 1);
	REQUIRE(gmst);
	CHECK(gmst->editorID == "sIn");
	CHECK(gmst->original == "In");

	const auto npc = store.Find(TypeOf("NPC_ FULL"), 0x40);
	REQUIRE(npc);
	REQUIRE(npc->conditions);
	CHECK(npc->conditions->size() == 1);
}

TEST_CASE("bad input only skips what is bad", "[store]")
{
	TempTree tree;
	tree.Write("Fallout4.esm/a-broken.json", R"([{"form_id": "00000001|Fallout4.esm", "type": )");
	tree.Write("Fallout4.esm/b.json", Array({
		Item("nonsense", "MISC FULL", "bad id"),
		Item("00000002|Fallout4.esm", "MISCFULL", "bad type"),
		Item("00000003|NotLoaded.esp", "MISC FULL", "plugin missing"),
		Item("00000004|Fallout4.esm", "MISC FULL", "good"),
	}));
	tree.Write("Fallout4.esm/notes.txt", "not json");

	TranslationStore store;
	const auto stats = store.Load(tree.Root(), kLoadOrder);

	CHECK(stats.filesFailed == 1);
	CHECK(stats.filesParsed == 1);
	CHECK(stats.entriesBadFormID == 1);
	CHECK(stats.entriesBadType == 1);
	CHECK(stats.entriesPluginNotLoaded == 1);
	CHECK(store.Size() == 1);
	CHECK(Text(store, TypeOf("MISC FULL"), 0x4));
	CHECK_FALSE(stats.errors.empty());
}

TEST_CASE("missing root folder loads nothing", "[store]")
{
	TranslationStore store;
	const auto stats = store.Load("Z:/definitely/not/here", kLoadOrder);
	CHECK(store.Size() == 0);
	CHECK(stats.filesParsed == 0);
}

TEST_CASE("reloading replaces previous data", "[store]")
{
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({ Item("00000001|Fallout4.esm", "MISC FULL", "A") }));
	TranslationStore store;
	store.Load(tree.Root(), kLoadOrder);
	REQUIRE(store.Size() == 1);

	std::filesystem::remove_all(tree.Root() / "Fallout4.esm");
	store.Load(tree.Root(), kLoadOrder);
	CHECK(store.Size() == 0);
}

TEST_CASE("debug info puts FormID and plugin in front of every text", "[store]")
{
	TempTree tree;
	tree.Write("Fallout4.esm/a.json", Array({ Item("0024A003|Fallout4.esm", "MISC FULL", "A") }));
	tree.Write("Patch.esp/a.json", Array({ Item("00000800|Patch.esp", "MISC FULL", "B") }));
	tree.Write("MyLight.esl/a.json", Array({ Item("FE002ABC|MyLight.esl", "MISC FULL", "C", 2) }));

	TranslationStore plain;
	plain.Load(tree.Root(), kLoadOrder);
	CHECK(*Text(plain, TypeOf("MISC FULL"), 0x0024A003) == "A");

	TranslationStore debug;
	debug.Load(tree.Root(), kLoadOrder, true);
	CHECK(*Text(debug, TypeOf("MISC FULL"), 0x0024A003) == "0024A003|Fallout4.esm|A");
	CHECK(*Text(debug, TypeOf("MISC FULL"), 0x02000800) == "02000800|Patch.esp|B");
	CHECK(*Text(debug, TypeOf("MISC FULL"), 0xFE002ABC, 2) == "FE002ABC|MyLight.esl|C");
}

TEST_CASE("debug prefix format", "[store]")
{
	CHECK(MakeDebugPrefix(0x0024A003, "Fallout4.esm") == "0024A003|Fallout4.esm|");
	CHECK(MakeDebugPrefix(0x1, "x.esp") == "00000001|x.esp|");
}
