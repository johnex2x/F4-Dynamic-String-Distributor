#include "PortedConstFields.h"

#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"
#include "TextWrite.h"

namespace PortedConstFields
{
	namespace
	{
		[[nodiscard]] constexpr Slot Written(bool a_changed) noexcept { return a_changed ? Slot::kApplied : Slot::kUnchanged; }

		// These fields have one value per form: the xTranslator index is 1 (0 in upstream DSD files)
		[[nodiscard]] constexpr bool IsSingleIndex(std::uint32_t a_index) noexcept { return a_index <= 1; }

		[[nodiscard]] bool SkipPlayer(const RE::TESForm* a_form)
		{
			return (a_form->IsPlayer() || a_form->IsPlayerRef()) && !Settings::GetSingleton().enablePlayerRef;
		}
	}

	// TESLoadScreen::loadingText, offset 0x30 (CommonLibF4 TESForms.h). ESM: LSCR DESC, 358 of 358 json entries match
	Slot WriteLoadScreenText(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto screen = a_form->As<RE::TESLoadScreen>();
		if (!screen) {
			return Slot::kWrongType;
		}
		if (!IsSingleIndex(a_index)) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(screen->loadingText, a_text));
	}

	// EffectSetting::magicItemDescription, offset 0x198 (CommonLibF4 TESForms.h)
	Slot WriteMagicDescription(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto effect = a_form->As<RE::EffectSetting>();
		if (!effect) {
			return Slot::kWrongType;
		}
		if (!IsSingleIndex(a_index)) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(effect->magicItemDescription, a_text));
	}

	// TESNPC::shortName, offset 0x298 (CommonLibF4 TESBoundAnimObjects.h)
	Slot WriteNpcShortName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto npc = a_form->As<RE::TESNPC>();
		if (!npc) {
			return Slot::kWrongType;
		}
		if (!IsSingleIndex(a_index)) {
			return Slot::kNoSlot;
		}
		if (SkipPlayer(a_form)) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(npc->shortName, a_text));
	}

	void ApplyNpcShortName(RE::TESForm* a_npc)
	{
		const auto& store = Manager::GetSingleton().Store();
		const auto  type = DSD::TypeOf("NPC_ SHRT");
		const auto  entry = store.Find(type, a_npc->GetFormID(), 1);
		if (const auto found = entry ? entry : store.Find(type, a_npc->GetFormID(), 0)) {
			static_cast<void>(WriteNpcShortName(a_npc, 1, found->text));
		}
	}

	// REFR FULL: the map marker's name lives in the ExtraMapMarker of the reference, not in a TESFullName
	// of the reference. MapMarkerData derives from TESFullName (CommonLibF4 BSExtraData.h);
	// FallHook src/110163/RuntimeTextReference.cpp does the same write on 1.10.163.
	Slot WriteMapMarkerName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto ref = a_form->As<RE::TESObjectREFR>();
		if (!ref) {
			return Slot::kWrongType;
		}
		if (!IsSingleIndex(a_index)) {
			return Slot::kNoSlot;
		}
		const auto extra = ref->extraList ? ref->extraList->GetByType<RE::ExtraMapMarker>() : nullptr;
		const auto marker = extra ? extra->mapMarkerData : nullptr;
		if (!marker) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(static_cast<RE::TESFullName*>(marker)->fullName, a_text));
	}

	namespace
	{
		// TESRegionDataList::Find(list, id): walks the list and returns the first data whose GetID() (vtable
		// slot 3) equals the id. OG Address Library ID 657534 from FallHook src/110163/RuntimeRegionMapText.cpp;
		// AE ID 2196228 (same shape and size in .191/.221/.240, docs/ae-independent-results.md).
		constexpr std::uint64_t kFindRegionDataIDOG = 657534;
		constexpr std::uint64_t kFindRegionDataIDAE = 2196228;
		// TESRegionDataMap::GetID returns 4 (AE exe 0x3EDD70: mov eax, 4), the same value as the RDAT type
		// 4 of the map data in Fallout4.esm REGN records
		constexpr std::uint32_t kMapDataID = 4;
		// TESRegionDataMap::mapName, offset 0x10 (FallHook src/110163/RuntimeRegionMapText.cpp)
		constexpr std::ptrdiff_t kMapNameOffset = 0x10;

		using FindRegionData_t = void*(RE::TESRegionDataList*, std::uint32_t);
	}

	// REGN RDMP: one map name per region
	Slot WriteRegionMapName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text)
	{
		const auto region = a_form->As<RE::TESRegion>();
		if (!region) {
			return Slot::kWrongType;
		}
		if (!IsSingleIndex(a_index) || !region->dataList) {
			return Slot::kNoSlot;
		}
		static REL::Relocation<FindRegionData_t> find{ REL::ID(DSD::IsAE() ? kFindRegionDataIDAE : kFindRegionDataIDOG) };
		const auto                               mapData = static_cast<std::byte*>(find(region->dataList, kMapDataID));
		if (!mapData) {
			return Slot::kNoSlot;
		}
		return Written(TextWrite::Assign(*reinterpret_cast<RE::BGSLocalizedString*>(mapData + kMapNameOffset), a_text));
	}

	namespace
	{
		// Perk entries are BGSPerkEntry subclasses with no layout in CommonLibF4. Layout read from the AE exe
		// (1.10.980 downgrade backup), checked against OG vtable slot pairs:
		//  - BGSEntryPointPerkEntry::Load (vtable slot 7, AE 0x623730): the EPFT subrecord creates the function data
		//    and stores it at +0x18; EPFD/EPF2/EPF3 are passed on to it.
		//  - BGSEntryPointFunctionDataActivateChoice::Load (vtable slot 5, AE 0x61A000): EPF2 loads the label
		//    (BGSLocalizedString) at +0x18; EPF3 stores its first u16 at +0x28 and its second u16 at +0x10.
		//    The object is 0x30 bytes.
		// The xTranslator index of PERK EPF2 is that first EPF3 u16 (Fallout4.esm: 0 or 2, 21 of 21 entries).
		// Reading +0x28 back before writing is the safety check: a wrong layout finds no match and writes nothing.
		// A mod can give several choices the same EPF3 index (Flashy_CrimeAndPunishment_GunForHire_Addon.esp: 3 choices,
		// all 0); an entry with "choice" then names its choice by position, counted in perkEntries order.
		constexpr std::ptrdiff_t kFunctionDataOffset = 0x18;
		constexpr std::ptrdiff_t kChoiceLabelOffset = 0x18;
		constexpr std::ptrdiff_t kChoiceIndexOffset = 0x28;
	}

	Slot WritePerkActivateChoice(RE::TESForm* a_form, std::uint32_t a_index, std::uint32_t a_choice, const std::string& a_text)
	{
		const auto perk = a_form->As<RE::BGSPerk>();
		if (!perk) {
			return Slot::kWrongType;
		}

		static const REL::Relocation<std::uintptr_t> entryVtable{ RE::VTABLE::BGSEntryPointPerkEntry[0] };
		static const REL::Relocation<std::uintptr_t> choiceVtable{ RE::VTABLE::BGSEntryPointFunctionDataActivateChoice[0] };

		auto          result = Slot::kNoSlot;
		std::uint32_t position = 0;  // Activate choices so far, 1-based
		for (const auto entry : perk->perkEntries) {
			if (!entry || *reinterpret_cast<const std::uintptr_t*>(entry) != entryVtable.address()) {
				continue;
			}
			const auto data = *reinterpret_cast<std::byte* const*>(reinterpret_cast<const std::byte*>(entry) + kFunctionDataOffset);
			if (!data || *reinterpret_cast<const std::uintptr_t*>(data) != choiceVtable.address()) {
				continue;
			}
			++position;
			if (a_choice && position != a_choice) {
				continue;
			}
			if (*reinterpret_cast<const std::uint16_t*>(data + kChoiceIndexOffset) == a_index) {
				// Several entries may share the index; one changed write makes the result "applied"
				const bool changed = TextWrite::Assign(*reinterpret_cast<RE::BGSLocalizedString*>(data + kChoiceLabelOffset), a_text);
				if (changed || result != Slot::kApplied) {
					result = Written(changed);
				}
			}
		}
		return result;
	}
}
