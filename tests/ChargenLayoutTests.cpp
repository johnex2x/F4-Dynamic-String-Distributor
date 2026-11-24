// BPTD part types and RACE per-sex counts: synthetic plugins, plus a local Fallout4.esm against
// Fallout4.json when both exist. Paths: env DSD_FALLOUT4_ESM and DSD_TRANSLATION_ROOT (see Fallout4JsonTests).
#include "core/ChargenLayout.h"
#include "core/TranslationStore.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <zlib.h>

using namespace DSD;

namespace
{
	using Bytes = std::vector<std::uint8_t>;

	void Append(Bytes& a_out, const void* a_data, std::size_t a_size)
	{
		const auto bytes = static_cast<const std::uint8_t*>(a_data);
		a_out.insert(a_out.end(), bytes, bytes + a_size);
	}

	template <class T>
	void Append(Bytes& a_out, T a_value)
	{
		Append(a_out, &a_value, sizeof(T));
	}

	Bytes Sub(const char* a_sig, const Bytes& a_data = {})
	{
		Bytes out;
		Append(out, a_sig, 4);
		Append(out, static_cast<std::uint16_t>(a_data.size()));
		Append(out, a_data.data(), a_data.size());
		return out;
	}

	Bytes Join(std::initializer_list<Bytes> a_parts)
	{
		Bytes out;
		for (const auto& part : a_parts) {
			Append(out, part.data(), part.size());
		}
		return out;
	}

	Bytes Record(const char* a_sig, std::uint32_t a_formID, const Bytes& a_body, bool a_compress = false)
	{
		Bytes         data = a_body;
		std::uint32_t flags = 0;
		if (a_compress) {
			uLongf size = compressBound(static_cast<uLong>(a_body.size()));
			Bytes  packed(size);
			REQUIRE(compress(packed.data(), &size, a_body.data(), static_cast<uLong>(a_body.size())) == Z_OK);
			packed.resize(size);
			data.clear();
			Append(data, static_cast<std::uint32_t>(a_body.size()));
			Append(data, packed.data(), packed.size());
			flags = 0x00040000;
		}

		Bytes out;
		Append(out, a_sig, 4);
		Append(out, static_cast<std::uint32_t>(data.size()));
		Append(out, flags);
		Append(out, a_formID);
		Append(out, std::uint64_t{ 0 });
		Append(out, data.data(), data.size());
		return out;
	}

	Bytes Group(const char* a_label, const Bytes& a_contents)
	{
		Bytes out;
		Append(out, "GRUP", 4);
		Append(out, static_cast<std::uint32_t>(a_contents.size() + 24));
		Append(out, a_label, 4);
		Append(out, std::int32_t{ 0 });
		Append(out, std::uint64_t{ 0 });
		Append(out, a_contents.data(), a_contents.size());
		return out;
	}

	Bytes Bpnd(std::uint8_t a_type)
	{
		Bytes data(101);
		data[65] = a_type;
		return Sub("BPND", data);
	}

	// Three parts: types 24, 9 and (no BPND) -> unknown
	Bytes BodyPartData()
	{
		return Join({ Sub("EDID", { 'B', 0 }),
			Sub("BPTN", Bytes(4)), Sub("BPNN", { 'a', 0 }), Bpnd(24),
			Sub("BPTN", Bytes(4)), Sub("BPNN", { 'b', 0 }), Bpnd(9),
			Sub("BPTN", Bytes(4)) });
	}

	// Male block: 3 TTGP, 1 MPPN, 2 FMRN. Female block: 2 TTGP, 1 MPPN, 2 FMRN.
	// MNAM / FNAM before the NAM0 markers (skeleton model markers) must not start a block.
	Bytes RaceData()
	{
		return Join({ Sub("EDID", { 'R', 0 }), Sub("MNAM"), Sub("FNAM"), Sub("TTGP", Bytes(4)),
			Sub("NAM0"), Sub("MNAM"),
			Sub("TTGP", Bytes(4)), Sub("TTGP", Bytes(4)), Sub("TTGP", Bytes(4)), Sub("MPPN", Bytes(4)), Sub("FMRN", Bytes(4)), Sub("FMRN", Bytes(4)),
			Sub("NAM0"), Sub("FNAM"),
			Sub("TTGP", Bytes(4)), Sub("TTGP", Bytes(4)), Sub("MPPN", Bytes(4)), Sub("FMRN", Bytes(4)), Sub("FMRN", Bytes(4)) });
	}

	// A plugin with one master: its own records have load-order byte 0x01
	std::istringstream Plugin(bool a_compress)
	{
		const auto tes4 = Record("TES4", 0, Join({ Sub("HEDR", Bytes(12)), Sub("MAST", { 'M', 0 }), Sub("DATA", Bytes(8)) }));
		const auto bodyParts = Group("BPTD", Join({ Record("BPTD", 0x01000800, BodyPartData(), a_compress), Record("BPTD", 0x00000123, BodyPartData()) }));
		const auto races = Group("RACE", Join({ Record("RACE", 0x01000900, RaceData(), a_compress), Record("RACE", 0x00000124, RaceData()) }));
		const auto bytes = Join({ tes4, bodyParts, races });
		return std::istringstream{ std::string{ bytes.begin(), bytes.end() }, std::ios::binary };
	}
}

TEST_CASE("body part types are listed in BPTN file order", "[chargen]")
{
	for (const bool compressed : { false, true }) {
		auto       plugin = Plugin(compressed);
		const auto index = ReadBodyPartTypes(plugin);

		REQUIRE(index.size() == 1);  // the override of a master record is not read
		CHECK(index.at(0x000800) == BodyPartTypes{ 24, 9, kNoPartType });
	}
}

TEST_CASE("race counts are kept per sex block", "[chargen]")
{
	for (const bool compressed : { false, true }) {
		auto       plugin = Plugin(compressed);
		const auto index = ReadRaceCounts(plugin);

		REQUIRE(index.size() == 1);
		const auto& counts = index.at(0x000900);
		CHECK(counts.tints == std::array<std::uint32_t, 2>{ 3, 2 });
		CHECK(counts.presets == std::array<std::uint32_t, 2>{ 1, 1 });
		CHECK(counts.regions == std::array<std::uint32_t, 2>{ 2, 2 });
	}
}

TEST_CASE("only the requested records are read", "[chargen]")
{
	const std::uint32_t other[]{ 0x000999 };
	auto                bodyParts = Plugin(false);
	CHECK(ReadBodyPartTypes(bodyParts, other).empty());
	auto races = Plugin(false);
	CHECK(ReadRaceCounts(races, other).empty());
}

TEST_CASE("a file without a TES4 header is rejected by the chargen readers", "[chargen]")
{
	std::istringstream junk{ std::string(64, 'x'), std::ios::binary };
	CHECK_THROWS(ReadBodyPartTypes(junk));
}

TEST_CASE("every BPTD / RACE chargen entry in Fallout4.json maps to the file layout of Fallout4.esm", "[integration]")
{
	const auto                  esmEnv = std::getenv("DSD_FALLOUT4_ESM");
	const std::filesystem::path esm = esmEnv && *esmEnv ? esmEnv : "";
	const auto                  rootEnv = std::getenv("DSD_TRANSLATION_ROOT");
	const std::filesystem::path root = rootEnv && *rootEnv ? rootEnv : DSD_DEFAULT_TRANSLATION_ROOT;
	if (esm.empty() || !std::filesystem::exists(esm) || !std::filesystem::exists(root / "Fallout4.esm" / "Fallout4.json")) {
		SKIP("set DSD_FALLOUT4_ESM to the game's Fallout4.esm to run this check");
	}

	TranslationStore store;
	const std::vector<PluginInfo> loadOrder{ { "Fallout4.esm", 0x00, 0, false } };
	store.Load(root, loadOrder);

	std::ifstream file{ esm, std::ios::binary };
	const auto    bodyParts = ReadBodyPartTypes(file);
	file.clear();
	file.seekg(0);
	const auto races = ReadRaceCounts(file);

	std::size_t entries = 0;
	std::size_t mapped = 0;
	store.ForEach(TypeOf("BPTD BPTN"), [&](std::uint32_t a_formID, std::uint32_t a_index, const Entry&) {
		++entries;
		const auto part = bodyParts.find(a_formID & 0x00FFFFFF);
		if (part != bodyParts.end() && a_index >= 1 && a_index <= part->second.size() && part->second[a_index - 1] < 26) {
			++mapped;
		}
	});
	CHECK(entries == 404);
	CHECK(mapped == entries);

	// The index runs 1..(male + female count), so every entry is inside the file's counts
	std::size_t raceEntries = 0;
	std::size_t raceMapped = 0;
	const auto  check = [&](const char* a_type, auto a_member) {
		const auto code = *MakeTypeCode(a_type);
		store.ForEach(code, [&](std::uint32_t a_formID, std::uint32_t a_index, const Entry&) {
			++raceEntries;
			const auto race = races.find(a_formID & 0x00FFFFFF);
			if (race != races.end()) {
				const auto& counts = (race->second).*a_member;
				if (a_index >= 1 && a_index <= counts[0] + counts[1]) {
					++raceMapped;
				}
			}
		});
	};
	check("RACE TTGP", &RaceCounts::tints);
	check("RACE MPPN", &RaceCounts::presets);
	check("RACE FMRN", &RaceCounts::regions);
	CHECK(raceEntries == 411 + 370 + 192);
	CHECK(raceMapped == raceEntries);
}
