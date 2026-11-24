// Player dialogue option text: which source wins, from form IDs only (ticket 06).
#include "core/DialogueOptions.h"

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
			_root = std::filesystem::temp_directory_path() / std::format("dsd-option-tests-{}-{}", std::chrono::steady_clock::now().time_since_epoch().count(), counter++);
			std::filesystem::remove_all(_root);
			std::filesystem::create_directories(_root / "Fallout4.esm");
		}
		~TempTree() { std::filesystem::remove_all(_root); }

		void Write(std::string_view a_content) const { std::ofstream{ _root / "Fallout4.esm" / "t.json", std::ios::binary } << a_content; }

		[[nodiscard]] const std::filesystem::path& Root() const noexcept { return _root; }

	private:
		std::filesystem::path _root;
	};

	const std::vector<PluginInfo> kLoadOrder{ { "Fallout4.esm", 0x00, 0, false } };

	// INFO 100 has RNAM and NAM1; INFO 200 only NAM1 (responses 1 and 2); INFO 300 only RNAM without index;
	// INFO 400 has nothing; DIAL 900 has FULL
	TranslationStore MakeStore()
	{
		static const TempTree tree;
		tree.Write(R"([
			{"form_id":"00000100|Fallout4.esm","type":"INFO RNAM","string":"prompt100","index":1},
			{"form_id":"00000100|Fallout4.esm","type":"INFO NAM1","string":"line100","index":1},
			{"form_id":"00000200|Fallout4.esm","type":"INFO NAM1","string":"line200a","index":1},
			{"form_id":"00000200|Fallout4.esm","type":"INFO NAM1","string":"line200b","index":2},
			{"form_id":"00000300|Fallout4.esm","type":"INFO RNAM","string":"prompt300"},
			{"form_id":"00000900|Fallout4.esm","type":"DIAL FULL","string":"topic900","index":1}
		])");
		TranslationStore store;
		store.Load(tree.Root(), kLoadOrder);
		return store;
	}

	const InfoLink kInfo100{ 0x100, 1, true };
	const InfoLink kInfo200{ 0x200, 1, true };
	const InfoLink kInfo300{ 0x300, 1, true };
	const InfoLink kInfo400{ 0x400, 1, true };
	const InfoLink kShared{ 0x500, 0, false };  // shared-response INFO: no own responses, dataInfo points on
}

TEST_CASE("wheel option: RNAM first, then DIAL FULL", "[dialogue-options]")
{
	const auto store = MakeStore();

	SECTION("RNAM with index 1")
	{
		const std::array chain{ kInfo100 };
		const auto       hit = ResolveWheelOption(store, chain, 0x900);
		REQUIRE(hit);
		CHECK(hit.source == OptionSource::kInfoRnam);
		CHECK(hit.entry->text == "prompt100");
		CHECK(hit.formID == 0x100);
	}
	SECTION("RNAM without index (upstream DSD files)")
	{
		const std::array chain{ kInfo300 };
		CHECK(ResolveWheelOption(store, chain, 0).entry->text == "prompt300");
	}
	SECTION("no RNAM falls back to the topic name, never to NAM1")
	{
		const std::array chain{ kInfo200 };
		const auto       hit = ResolveWheelOption(store, chain, 0x900);
		REQUIRE(hit);
		CHECK(hit.source == OptionSource::kDialFull);
		CHECK(hit.entry->text == "topic900");
		CHECK(hit.formID == 0x900);
	}
	SECTION("nothing translated")
	{
		const std::array chain{ kInfo400 };
		CHECK_FALSE(ResolveWheelOption(store, chain, 0x901));
		CHECK_FALSE(ResolveWheelOption(store, chain, 0));
	}
	SECTION("shared-response INFO: RNAM of a later INFO in the dataInfo chain is found")
	{
		const std::array chain{ kShared, kInfo100 };
		CHECK(ResolveWheelOption(store, chain, 0).entry->text == "prompt100");
	}
	SECTION("empty chain still reaches the topic name")
	{
		CHECK(ResolveWheelOption(store, {}, 0x900).entry->text == "topic900");
	}
}

TEST_CASE("XDI option: prompt from RNAM, response from the owned NAM1", "[dialogue-options]")
{
	const auto store = MakeStore();

	SECTION("both set when both exist")
	{
		const std::array chain{ kInfo100 };
		const auto       option = ResolveXdiOption(store, chain);
		REQUIRE(option.prompt);
		CHECK(option.prompt.entry->text == "prompt100");
		REQUIRE(option.response);
		CHECK(option.response.entry->text == "line100");
		CHECK(option.response.source == OptionSource::kInfoNam1);
	}
	SECTION("no RNAM: only the response is replaced, DIAL FULL is not used")
	{
		const std::array chain{ kInfo200 };
		const auto       option = ResolveXdiOption(store, chain);
		CHECK_FALSE(option.prompt);
		REQUIRE(option.response);
		CHECK(option.response.entry->text == "line200a");  // responseID 1, the first response
	}
	SECTION("responseID of the first response selects the NAM1")
	{
		const std::array chain{ InfoLink{ 0x200, 2, true } };
		CHECK(ResolveXdiOption(store, chain).response.entry->text == "line200b");
	}
	SECTION("shared-response INFO: NAM1 is keyed by the INFO that owns the responses")
	{
		const std::array chain{ kShared, kInfo200 };
		const auto       option = ResolveXdiOption(store, chain);
		REQUIRE(option.response);
		CHECK(option.response.formID == 0x200);
		CHECK(option.response.entry->text == "line200a");
	}
	SECTION("a shared-response INFO itself is never a NAM1 key")
	{
		// Even if the store had NAM1 for 0x500, an INFO without responses owns none
		const std::array chain{ kShared };
		CHECK_FALSE(ResolveXdiOption(store, chain));
	}
	SECTION("the first owner decides: no fallback to a later INFO's NAM1")
	{
		const std::array chain{ kInfo400, kInfo200 };
		CHECK_FALSE(ResolveXdiOption(store, chain).response);
	}
	SECTION("nothing translated")
	{
		const std::array chain{ kInfo400 };
		CHECK_FALSE(ResolveXdiOption(store, chain));
	}
}

TEST_CASE("option source names", "[dialogue-options]")
{
	CHECK(OptionSourceName(OptionSource::kInfoRnam) == "INFO RNAM");
	CHECK(OptionSourceName(OptionSource::kInfoNam1) == "INFO NAM1");
	CHECK(OptionSourceName(OptionSource::kDialFull) == "DIAL FULL");
	CHECK(OptionSourceName(OptionSource::kNone) == "none");
}
