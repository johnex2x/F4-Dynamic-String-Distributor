#pragma once

// File layout of BPTD (VATS body parts) and RACE (character creation) records, read from the plugin file.
// The JSON index of BPTD BPTN / RACE TTGP / MPPN / FMRN is the 1-based position of the subrecord in the
// record. The game keeps these lists in other shapes (parts by part type, face data by sex), so the
// plugin file is read once to translate between the two. No game objects are touched, and no text is compared.

#include <array>
#include <cstdint>
#include <istream>
#include <span>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

namespace DSD
{
	// Part type (BGSBodyPartData::PartType, the index into partArray) of the n-th BPTN; 0xFF when the part has no BPND.
	// BPND is 101 bytes; byte 65 is PART_DATA::type (runtime offset 0x95; Fallout4.esm: first part of
	// RaiderDogBodyPartData "Right Foot" = 24 = RightFoot, second "Left Foot" = 23 = LeftFoot).
	constexpr std::uint8_t kNoPartType = 0xFF;
	using BodyPartTypes = std::vector<std::uint8_t>;

	// Low 24 bits of the BPTD form ID -> part types in file order
	using BodyPartIndex = boost::unordered_flat_map<std::uint32_t, BodyPartTypes>;

	// Number of TTGP / MPPN / FMRN subrecords in each sex block. A sex block starts at the NAM0 + MNAM (male)
	// or NAM0 + FNAM (female) marker pair; the index counts the male block first, then the female block.
	struct RaceCounts
	{
		std::array<std::uint32_t, 2> tints{};    // TTGP: tint group names and tint option names
		std::array<std::uint32_t, 2> presets{};  // MPPN: face preset names
		std::array<std::uint32_t, 2> regions{};  // FMRN: face region names
	};
	using RaceIndex = boost::unordered_flat_map<std::uint32_t, RaceCounts>;

	// Read the records that the plugin itself defines (not overrides of its masters).
	// a_forms: low 24 bits of the records to read; empty reads every record.
	// Throw std::runtime_error on a malformed file.
	[[nodiscard]] BodyPartIndex ReadBodyPartTypes(std::istream& a_plugin, std::span<const std::uint32_t> a_forms = {});
	[[nodiscard]] RaceIndex     ReadRaceCounts(std::istream& a_plugin, std::span<const std::uint32_t> a_forms = {});
}
