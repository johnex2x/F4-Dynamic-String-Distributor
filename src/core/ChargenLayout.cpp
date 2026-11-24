#include "core/ChargenLayout.h"

#include "PluginFile.h"

using namespace DSD::PluginFile;

namespace DSD
{
	BodyPartIndex ReadBodyPartTypes(std::istream& a_plugin, std::span<const std::uint32_t> a_forms)
	{
		constexpr std::size_t kTypeOffset = 65;

		BodyPartIndex index;
		ForEachOwnRecord(a_plugin, "BPTD", a_forms, [&](std::uint32_t a_low, const std::vector<std::uint8_t>& a_data) {
			BodyPartTypes types;
			ForEachSubrecord(a_data, [&](std::string_view a_sig, const std::uint8_t* a_sub, std::size_t a_size) {
				if (a_sig == "BPTN") {
					types.push_back(kNoPartType);
				} else if (a_sig == "BPND" && !types.empty() && a_size > kTypeOffset) {
					types.back() = a_sub[kTypeOffset];
				}
			});
			index.insert_or_assign(a_low, std::move(types));
		});
		return index;
	}

	RaceIndex ReadRaceCounts(std::istream& a_plugin, std::span<const std::uint32_t> a_forms)
	{
		RaceIndex index;
		ForEachOwnRecord(a_plugin, "RACE", a_forms, [&](std::uint32_t a_low, const std::vector<std::uint8_t>& a_data) {
			RaceCounts                   counts;
			std::optional<std::size_t>   sex;
			bool                         afterNAM0 = false;
			ForEachSubrecord(a_data, [&](std::string_view a_sig, const std::uint8_t*, std::size_t) {
				if (afterNAM0 && (a_sig == "MNAM" || a_sig == "FNAM")) {
					sex = a_sig == "MNAM" ? 0 : 1;
				} else if (sex && a_sig == "TTGP") {
					++counts.tints[*sex];
				} else if (sex && a_sig == "MPPN") {
					++counts.presets[*sex];
				} else if (sex && a_sig == "FMRN") {
					++counts.regions[*sex];
				}
				afterNAM0 = a_sig == "NAM0";
			});
			index.insert_or_assign(a_low, counts);
		});
		return index;
	}
}
