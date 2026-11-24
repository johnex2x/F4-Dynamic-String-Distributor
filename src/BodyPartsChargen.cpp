#include "BodyPartsChargen.h"

#include "Manager.h"
#include "Runtime.h"
#include "TextWrite.h"
#include "core/ChargenLayout.h"

// Index rules (checked against Fallout4.esm and Fallout4.json, see ticket 12). The index is always the 1-based
// position of the subrecord in its record. No text is compared: the game's own strings may be in another
// language than the JSON "original".
//   BPTD BPTN : the n-th BPTN. BGSBodyPartData keeps its parts in partArray[26], indexed by part type, so the
//               plugin file is read for the part type (BPND) of the n-th BPTN (same idea as QUST CNAM numbers).
//   RACE TTGP : the n-th TTGP, male block then female block. A TTGP is either a tint group name or the name of
//               a tint option inside the group: group, option, option, ..., next group.
//   RACE MPPN : the n-th face preset name, male block then female block, group after group.
//   RACE FMRN : the n-th face region name, male block then female block.
// For RACE the plugin file is also read for the number of these subrecords per sex. If the game holds a
// different number for a race, that race is skipped for that type rather than risk writing to the wrong slot.
namespace
{
	constexpr auto kBodyPartName = DSD::TypeOf("BPTD BPTN");
	constexpr auto kTintName = DSD::TypeOf("RACE TTGP");
	constexpr auto kFacePreset = DSD::TypeOf("RACE MPPN");
	constexpr auto kFaceRegion = DSD::TypeOf("RACE FMRN");

	struct ApplyStats
	{
		std::size_t applied{ 0 };         // includes the unchanged ones
		std::size_t unchanged{ 0 };       // part of applied: the slot already held the text
		std::size_t formMissing{ 0 };
		std::size_t wrongType{ 0 };       // form is not a BPTD / RACE
		std::size_t noLayout{ 0 };        // the plugin file has no such record (or cannot be read)
		std::size_t countMismatch{ 0 };   // RACE: the game holds another number of names than the file
		std::size_t noSlot{ 0 };          // index points past the list, or the part does not exist
	};

	struct Layouts
	{
		DSD::BodyPartIndex bodyParts;  // key: full runtime form ID
		DSD::RaceIndex     races;
	};

	// Reads the BPTD and RACE records of every plugin that defines a translated form
	Layouts ReadLayouts(const DSD::TranslationStore& a_store)
	{
		struct Wanted
		{
			std::vector<std::uint32_t> bodyParts;
			std::vector<std::uint32_t> races;
		};
		std::map<RE::TESFile*, Wanted> byFile;

		const auto collect = [&](DSD::TypeCode a_type, auto a_member) {
			a_store.ForEach(a_type, [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry&) {
				const auto form = DSD::LookupFormByID(a_formID);
				const auto file = form ? form->GetFile(0) : nullptr;
				if (!file) {
					return;
				}
				auto& ids = byFile[file].*a_member;
				if (std::ranges::find(ids, a_formID) == ids.end()) {
					ids.push_back(a_formID);
				}
			});
		};
		collect(kBodyPartName, &Wanted::bodyParts);
		collect(kTintName, &Wanted::races);
		collect(kFacePreset, &Wanted::races);
		collect(kFaceRegion, &Wanted::races);

		const auto low = [](const std::vector<std::uint32_t>& a_ids) {
			std::vector<std::uint32_t> result;
			for (const auto id : a_ids) {
				result.push_back(DSD::ToFileLocalFormID(id));
			}
			return result;
		};

		Layouts layouts;
		for (const auto& [file, wanted] : byFile) {
			const auto    path = std::filesystem::path{ "Data" } / file->GetFilename();
			std::ifstream stream{ path, std::ios::binary };
			if (!stream) {
				logger::error("BPTD / RACE: cannot open {}", path.string());
				continue;
			}
			try {
				if (!wanted.bodyParts.empty()) {
					auto index = DSD::ReadBodyPartTypes(stream, low(wanted.bodyParts));
					for (const auto formID : wanted.bodyParts) {
						if (const auto it = index.find(DSD::ToFileLocalFormID(formID)); it != index.end()) {
							layouts.bodyParts.insert_or_assign(formID, std::move(it->second));
						}
					}
				}
				if (!wanted.races.empty()) {
					stream.clear();
					stream.seekg(0);
					const auto index = DSD::ReadRaceCounts(stream, low(wanted.races));
					for (const auto formID : wanted.races) {
						if (const auto it = index.find(DSD::ToFileLocalFormID(formID)); it != index.end()) {
							layouts.races.insert_or_assign(formID, it->second);
						}
					}
				}
			} catch (const std::exception& e) {
				logger::error("BPTD / RACE: cannot read {}: {}", path.string(), e.what());
			}
		}
		return layouts;
	}

	using Slot = RE::BGSLocalizedString*;
	using SexSlots = std::array<std::vector<Slot>, 2>;  // male, female (TESRace::faceRelatedData[0], [1])

	// 1-based list position; 0 (no index in the JSON) means the first one
	[[nodiscard]] constexpr std::size_t Position(std::uint32_t a_index) noexcept { return a_index ? a_index - 1 : 0; }

	SexSlots TintSlots(RE::TESRace* a_race)
	{
		SexSlots slots;
		for (std::size_t sex = 0; sex < 2; ++sex) {
			const auto face = a_race->faceRelatedData[sex];
			if (!face || !face->tintingTemplate) {
				continue;
			}
			for (const auto group : face->tintingTemplate->groups) {
				if (!group) {
					continue;
				}
				slots[sex].push_back(&group->name);
				for (const auto entry : group->entries) {
					if (entry) {
						slots[sex].push_back(&entry->name);
					}
				}
			}
		}
		return slots;
	}

	SexSlots FacePresetSlots(RE::TESRace* a_race)
	{
		SexSlots slots;
		for (std::size_t sex = 0; sex < 2; ++sex) {
			const auto face = a_race->faceRelatedData[sex];
			if (!face || !face->morphGroups) {
				continue;
			}
			for (const auto group : *face->morphGroups) {
				if (!group) {
					continue;
				}
				for (auto& preset : group->presets) {
					slots[sex].push_back(&preset.name);
				}
			}
		}
		return slots;
	}

	SexSlots FaceRegionSlots(RE::TESRace* a_race)
	{
		SexSlots slots;
		for (std::size_t sex = 0; sex < 2; ++sex) {
			const auto face = a_race->faceRelatedData[sex];
			if (!face || !face->facialBoneRegions) {
				continue;
			}
			for (const auto region : *face->facialBoneRegions) {
				if (region) {
					slots[sex].push_back(&region->name);
				}
			}
		}
		return slots;
	}

	// Returns false when the slot already held the text
	bool Write(Slot a_slot, const DSD::Entry& a_entry, std::uint32_t a_formID, DSD::TypeCode a_type, std::uint32_t a_index)
	{
		if (TextWrite::Holds(*a_slot, a_entry.text)) {
			return false;
		}
		logger::debug("{} {:08X} #{}: '{}' -> '{}' (original '{}')", DSD::TypeCodeToString(a_type), a_formID, a_index,
			static_cast<std::string_view>(*a_slot), a_entry.text, a_entry.original.value_or(""));
		*a_slot = a_entry.text;
		return true;
	}

	// What ReadLayouts found in the plugin files. The files do not change while the game data stays loaded,
	// so the later Apply calls (after a save load) reuse it. Reset() drops it before a data reload.
	std::optional<Layouts> g_layouts;

	void ApplyBodyParts(const DSD::TranslationStore& a_store, const Layouts& a_layouts, ApplyStats& a_stats)
	{
		a_store.ForEach(kBodyPartName, [&](std::uint32_t a_formID, std::uint32_t a_index, const DSD::Entry& a_entry) {
			const auto form = DSD::LookupFormByID(a_formID);
			if (!form) {
				++a_stats.formMissing;
				return;
			}
			const auto data = form->As<RE::BGSBodyPartData>();
			if (!data) {
				++a_stats.wrongType;
				logger::debug("BPTD BPTN {:08X}: form type {} is not a body part data", a_formID, form->GetFormTypeString());
				return;
			}
			const auto layout = a_layouts.bodyParts.find(a_formID);
			if (layout == a_layouts.bodyParts.end()) {
				++a_stats.noLayout;
				logger::debug("BPTD BPTN {:08X}: record not found in its plugin file", a_formID);
				return;
			}

			const auto position = Position(a_index);
			const auto type = position < layout->second.size() ? layout->second[position] : DSD::kNoPartType;
			const auto part = type < std::size(data->partArray) ? data->partArray[type] : nullptr;
			if (!part) {
				++a_stats.noSlot;
				logger::debug("BPTD BPTN {:08X}: index {} has no part (part type {})", a_formID, a_index, type);
				return;
			}
			a_stats.unchanged += !Write(&part->partName, a_entry, a_formID, kBodyPartName, a_index);
			++a_stats.applied;
		});
	}

	template <class Build, class Counts>
	void ApplyRace(const DSD::TranslationStore& a_store, const Layouts& a_layouts, DSD::TypeCode a_type, ApplyStats& a_stats, Build a_build, Counts a_counts)
	{
		struct Cached
		{
			enum class Status { kOk, kMissing, kWrongType, kNoLayout, kMismatch } status{ Status::kMissing };
			SexSlots slots;
		};
		std::unordered_map<std::uint32_t, Cached> cache;

		a_store.ForEach(a_type, [&](std::uint32_t a_formID, std::uint32_t a_index, const DSD::Entry& a_entry) {
			auto it = cache.find(a_formID);
			if (it == cache.end()) {
				Cached cached;
				if (const auto form = DSD::LookupFormByID(a_formID)) {
					const auto race = form->As<RE::TESRace>();
					const auto layout = a_layouts.races.find(a_formID);
					if (!race) {
						cached.status = Cached::Status::kWrongType;
						logger::debug("{} {:08X}: form type {} is not a race", DSD::TypeCodeToString(a_type), a_formID, form->GetFormTypeString());
					} else if (layout == a_layouts.races.end()) {
						cached.status = Cached::Status::kNoLayout;
						logger::debug("{} {:08X}: record not found in its plugin file", DSD::TypeCodeToString(a_type), a_formID);
					} else {
						cached.slots = a_build(race);
						const auto& expected = a_counts(layout->second);
						if (cached.slots[0].size() == expected[0] && cached.slots[1].size() == expected[1]) {
							cached.status = Cached::Status::kOk;
						} else {
							cached.status = Cached::Status::kMismatch;
							logger::warn("{} {:08X}: skipped, the game has {} male + {} female names but the plugin file has {} + {}",
								DSD::TypeCodeToString(a_type), a_formID, cached.slots[0].size(), cached.slots[1].size(), expected[0], expected[1]);
						}
					}
				}
				it = cache.emplace(a_formID, std::move(cached)).first;
			}

			const auto& cached = it->second;
			switch (cached.status) {
			case Cached::Status::kMissing:
				++a_stats.formMissing;
				return;
			case Cached::Status::kWrongType:
				++a_stats.wrongType;
				return;
			case Cached::Status::kNoLayout:
				++a_stats.noLayout;
				return;
			case Cached::Status::kMismatch:
				++a_stats.countMismatch;
				return;
			case Cached::Status::kOk:
				break;
			}

			// Male names first, then female names: the order of the subrecords in the file
			auto position = Position(a_index);
			for (const auto& list : cached.slots) {
				if (position < list.size()) {
					a_stats.unchanged += !Write(list[position], a_entry, a_formID, a_type, a_index);
					++a_stats.applied;
					return;
				}
				position -= list.size();
			}
			++a_stats.noSlot;
			logger::debug("{} {:08X}: index {} is past the last name", DSD::TypeCodeToString(a_type), a_formID, a_index);
		});
	}
}

namespace BodyPartsChargen
{
	void Reset()
	{
		g_layouts.reset();
	}

	void Apply()
	{
		const auto  start = std::chrono::steady_clock::now();
		const auto& store = Manager::GetSingleton().Store();

		const bool cached = g_layouts.has_value();
		if (!cached) {
			g_layouts = ReadLayouts(store);
		}
		const auto&                       layouts = *g_layouts;
		std::map<std::string, ApplyStats> stats;
		ApplyBodyParts(store, layouts, stats["BPTD BPTN"]);
		ApplyRace(store, layouts, kTintName, stats["RACE TTGP"], TintSlots, [](const DSD::RaceCounts& a_counts) -> const auto& { return a_counts.tints; });
		ApplyRace(store, layouts, kFacePreset, stats["RACE MPPN"], FacePresetSlots, [](const DSD::RaceCounts& a_counts) -> const auto& { return a_counts.presets; });
		ApplyRace(store, layouts, kFaceRegion, stats["RACE FMRN"], FaceRegionSlots, [](const DSD::RaceCounts& a_counts) -> const auto& { return a_counts.regions; });

		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
		logger::info("Body part and chargen names applied in {} ms (plugin files {}):", elapsed.count(), cached ? "cached" : "read");
		for (const auto& [name, s] : stats) {
			logger::info("  {} : {:>6} applied ({} unchanged), {:>4} form missing, {:>4} wrong type, {:>4} no file record, {:>4} count mismatch, {:>4} no slot",
				name, s.applied, s.unchanged, s.formMissing, s.wrongType, s.noLayout, s.countMismatch, s.noSlot);
		}
	}
}
