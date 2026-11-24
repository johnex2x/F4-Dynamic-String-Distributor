// QUST CNAM numbering: synthetic plugins, plus a local Fallout4.esm against Fallout4.json when both exist.
// Paths: env DSD_FALLOUT4_ESM (the game's Fallout4.esm) and DSD_TRANSLATION_ROOT (see Fallout4JsonTests).
#include "core/QuestLogNumbers.h"
#include "core/TranslationStore.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <zlib.h>

#include <boost/unordered/unordered_flat_map.hpp>

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

	Bytes U16(std::uint16_t a_value)
	{
		Bytes out;
		Append(out, a_value);
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

	Bytes Record(const char* a_sig, std::uint32_t a_formID, const Bytes& a_body, bool a_compress = false, std::uint32_t a_flags = 0)
	{
		Bytes data = a_body;
		std::uint32_t flags = a_flags;
		if (a_compress) {
			uLongf size = compressBound(static_cast<uLong>(a_body.size()));
			Bytes  packed(size);
			REQUIRE(compress(packed.data(), &size, a_body.data(), static_cast<uLong>(a_body.size())) == Z_OK);
			packed.resize(size);
			data.clear();
			Append(data, static_cast<std::uint32_t>(a_body.size()));
			Append(data, packed.data(), packed.size());
			flags |= 0x00040000;
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

	Bytes Group(const char* a_label, std::int32_t a_type, const Bytes& a_contents)
	{
		Bytes out;
		Append(out, "GRUP", 4);
		Append(out, static_cast<std::uint32_t>(a_contents.size() + 24));
		Append(out, a_label, 4);
		Append(out, a_type);
		Append(out, std::uint64_t{ 0 });
		Append(out, a_contents.data(), a_contents.size());
		return out;
	}

	Bytes Text(const char* a_text)
	{
		Bytes out;
		Append(out, a_text, std::strlen(a_text) + 1);
		return out;
	}

	Bytes StringID(std::uint32_t a_id)
	{
		Bytes out;
		Append(out, a_id);
		return out;
	}

	// Stage 10: entry 0 has text, entry 1 has no CNAM, entry 2 has a blank CNAM, entry 3 has text.
	// Stage 20: one entry with text, then an objective. Numbers: (10,0)=1, (10,3)=2, (20,0)=3.
	// A localized plugin stores string IDs, where 0 is blank.
	Bytes QuestBody(bool a_localized)
	{
		const auto cnam = [&](const char* a_text, std::uint32_t a_id) { return Sub("CNAM", a_localized ? StringID(a_id) : Text(a_text)); };
		return Join({ Sub("EDID", { 'Q', 0 }),
			Sub("INDX", U16(10)), Sub("QSDT", { 0 }), cnam("A", 7), Sub("QSDT", { 0 }), Sub("QSDT", { 0 }), cnam(" \t\r\n", 0), Sub("QSDT", { 0 }), cnam("B", 9),
			Sub("INDX", U16(20)), Sub("QSDT", { 0 }), Sub("CTDA", Bytes(32)), cnam("C", 11),
			Sub("QOBJ", U16(10)), Sub("QSDT", { 0 }), Sub("NNAM", { 4, 0, 0, 0 }) });
	}

	// A plugin with one master: its own quests have load-order byte 0x01
	std::istringstream Plugin(bool a_compress, bool a_localized)
	{
		const auto tes4 = Record("TES4", 0, Join({ Sub("HEDR", Bytes(12)), Sub("MAST", { 'M', 0 }), Sub("DATA", Bytes(8)) }), false, a_localized ? 0x80 : 0);
		const auto weapons = Group("WEAP", 0, Record("WEAP", 0x01000900, Sub("EDID", { 'W', 0 })));
		const auto quests = Group("QUST", 0, Join({
			Record("QUST", 0x01000800, QuestBody(a_localized), a_compress),
			Group("\x00\x08\x00\x01", 10, Record("DIAL", 0x01000801, Sub("EDID", { 'D', 0 }))),
			Record("QUST", 0x00000123, QuestBody(a_localized)),  // override of a master quest
		}));
		const auto bytes = Join({ tes4, weapons, quests });
		return std::istringstream{ std::string{ bytes.begin(), bytes.end() }, std::ios::binary };
	}
}

TEST_CASE("only entries with journal text are numbered, across stages in file order", "[quest-log]")
{
	for (const bool localized : { false, true }) {
		for (const bool compressed : { false, true }) {
			auto       plugin = Plugin(compressed, localized);
			const auto index = ReadQuestLogNumbers(plugin);

			REQUIRE(index.size() == 1);
			const auto& quest = index.at(0x000800);
			CHECK(quest.Size() == 3);
			CHECK(quest.Find(10, 0) == 1u);
			CHECK_FALSE(quest.Find(10, 1));  // no CNAM
			CHECK_FALSE(quest.Find(10, 2));  // blank CNAM
			CHECK(quest.Find(10, 3) == 2u);  // the skipped entries did not advance the count
			CHECK(quest.Find(20, 0) == 3u);
			CHECK_FALSE(quest.Find(20, 1));
			CHECK_FALSE(quest.Find(30, 0));
		}
	}
}

TEST_CASE("only the requested quests are read", "[quest-log]")
{
	auto                 plugin = Plugin(false, false);
	const std::uint32_t  other[]{ 0x000999 };
	CHECK(ReadQuestLogNumbers(plugin, other).empty());
}

TEST_CASE("a file without a TES4 header is rejected", "[quest-log]")
{
	std::istringstream junk{ std::string(64, 'x'), std::ios::binary };
	CHECK_THROWS(ReadQuestLogNumbers(junk));
}

TEST_CASE("every QUST CNAM entry in Fallout4.json maps to a journal entry in Fallout4.esm", "[integration]")
{
	const auto esmEnv = std::getenv("DSD_FALLOUT4_ESM");
	const std::filesystem::path esm = esmEnv && *esmEnv ? esmEnv : "";
	const auto rootEnv = std::getenv("DSD_TRANSLATION_ROOT");
	const std::filesystem::path root = rootEnv && *rootEnv ? rootEnv : DSD_DEFAULT_TRANSLATION_ROOT;
	if (esm.empty() || !std::filesystem::exists(esm) || !std::filesystem::exists(root / "Fallout4.esm" / "Fallout4.json")) {
		SKIP("set DSD_FALLOUT4_ESM to the game's Fallout4.esm to run this check");
	}

	TranslationStore store;
	const std::vector<PluginInfo> loadOrder{ { "Fallout4.esm", 0x00, 0, false } };
	store.Load(root, loadOrder);

	std::ifstream file{ esm, std::ios::binary };
	const auto    index = ReadQuestLogNumbers(file);

	std::size_t entries = 0;
	std::size_t mapped = 0;
	boost::unordered_flat_map<std::uint32_t, std::size_t> perQuest;  // JSON entries of each quest
	store.ForEach(TypeOf("QUST CNAM"), [&](std::uint32_t a_formID, std::uint32_t a_index, const Entry&) {
		++entries;
		// Numbers run 1..Size() without gaps, so the index is mapped when it is in that range
		const auto quest = index.find(a_formID & 0x00FFFFFF);
		if (quest != index.end() && a_index >= 1 && a_index <= quest->second.Size()) {
			++mapped;
		}
		++perQuest[a_formID & 0x00FFFFFF];
	});

	CHECK(entries == 919);
	CHECK(mapped == entries);

	// Numbers are assigned to non-blank CNAMs only, so a quest has exactly as many numbers as JSON entries
	std::size_t mismatched = 0;
	for (const auto& [quest, count] : perQuest) {
		const auto it = index.find(quest);
		if (quest != 0x05DDAB && (it == index.end() || it->second.Size() != count)) {  // BoS100: known gap, see ParseQuest
			++mismatched;
		}
	}
	CHECK(mismatched == 0);
}
