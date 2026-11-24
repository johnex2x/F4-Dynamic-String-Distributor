// Integration test against a real Fallout4.esm translation file.
// Path: env DSD_TRANSLATION_ROOT (the DynamicStringDistributor folder), else the default set at configure time.
#include "core/HandledTypes.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <set>

using namespace DSD;

namespace
{
	std::filesystem::path TranslationRoot()
	{
		if (const auto env = std::getenv("DSD_TRANSLATION_ROOT"); env && *env) {
			return env;
		}
		return DSD_DEFAULT_TRANSLATION_ROOT;
	}
}

TEST_CASE("local Fallout4.json loads completely", "[integration]")
{
	const auto root = TranslationRoot();
	if (!std::filesystem::exists(root / "Fallout4.esm" / "Fallout4.json")) {
		SKIP("Fallout4.json not found under " << root.string());
	}

	const std::vector<PluginInfo> loadOrder{ { "Fallout4.esm", 0x00, 0, false } };

	TranslationStore store;
	const auto       start = std::chrono::steady_clock::now();
	const auto       stats = store.Load(root, loadOrder);
	const auto       elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

	std::size_t stored = 0;
	std::size_t shadowed = 0;
	for (const auto& [type, typeStats] : stats.types) {
		stored += typeStats.stored;
		shadowed += typeStats.shadowed;
	}

	std::cout << std::format("Fallout4.json: {} entries, {} stored, {} shadowed, {} types, {} ms\n",
		stats.entriesRead, stored, shadowed, stats.types.size(), elapsed.count());

	CHECK(stats.filesParsed == 1);
	CHECK(stats.filesFailed == 0);
	CHECK(stats.entriesRead == 118'799);
	CHECK(stats.entriesBadFormID == 0);
	CHECK(stats.entriesBadType == 0);
	CHECK(stats.entriesPluginNotLoaded == 0);
	CHECK(stored + shadowed == 118'799);
	CHECK(stats.types.at(TypeOf("INNR WNAM")).noOriginal == 0);
	CHECK(stats.types.size() == 90);
	CHECK(store.Size() == stored);

	// Spot checks from the file
	const auto gmst = store.Find(TypeOf("GMST DATA"), 0x0024A003, 1);
	REQUIRE(gmst);
	CHECK(gmst->editorID == "sWorkshopMenuLogicText");
	CHECK(gmst->text == "輸電邏輯閘");
}

TEST_CASE("every type in a local Fallout4.json is handled", "[integration]")
{
	const auto root = TranslationRoot();
	if (!std::filesystem::exists(root / "Fallout4.esm" / "Fallout4.json")) {
		SKIP("Fallout4.json not found under " << root.string());
	}

	const std::vector<PluginInfo> loadOrder{ { "Fallout4.esm", 0x00, 0, false } };

	TranslationStore store;
	const auto       stats = store.Load(root, loadOrder);

	std::set<std::string> unhandled;
	std::size_t           unhandledEntries = 0;
	for (const auto& [type, typeStats] : stats.types) {
		if (!IsHandledType(type)) {
			unhandled.insert(TypeCodeToString(type));
			unhandledEntries += typeStats.stored;
		}
	}

	// LVLI ONAM (87 entries) used to be the one known gap; LeveledListNames hooks its reader (ticket 14).
	// Every type in the file is handled now. This test fails if any type appears that nothing handles.
	CHECK(unhandled.empty());
	CHECK(unhandledEntries == 0);
}

TEST_CASE("IsHandledType covers the rule based and the listed types", "[handled]")
{
	CHECK(IsHandledType(TypeOf("ARMO FULL")));  // base name rule
	CHECK(IsHandledType(TypeOf("REFR FULL")));  // listed: map markers
	CHECK(IsHandledType(TypeOf("ALCH DESC")));  // description rule
	CHECK(IsHandledType(TypeOf("LSCR DESC")));  // listed: load screens
	CHECK(IsHandledType(TypeOf("INGR YNAM")));  // not display text, but counted as skipped
	CHECK(IsHandledType(TypeOf("LVLI ONAM")));  // vtable hook on the override name reader
}
