#include "MiscTypes.h"

#include "Manager.h"
#include "Runtime.h"
#include "TextWrite.h"

// Fallout 4 single-field text types (ticket 14). Every type here has exactly one subrecord per
// record in Fallout4.esm, and the JSON uses index 1 (0 in upstream-style files); the index is ignored.
//
// Not handled here (see ticket 14):
// - ACTI/FLOR/FURN ATTX: the game keeps the text in a global map; ActivateText.cpp replaces it on the HUD rollover.
// - LVLI ONAM: the override name lives in a global map with no OG Address Library ID; LeveledListNames.cpp hooks its reader.
// - INGR YNAM: not display text (pickup sound), counted as skipped.
namespace
{
	struct ApplyStats
	{
		std::size_t applied{ 0 };    // includes the unchanged ones
		std::size_t unchanged{ 0 };  // part of applied: the field already held the text
		std::size_t formMissing{ 0 };
		std::size_t wrongType{ 0 };  // the form has no such field
		std::size_t skipped{ 0 };    // not display text, nothing to write
	};

	enum class Outcome
	{
		kWrongType,  // the form has no such field
		kChanged,
		kUnchanged  // the field already held the text
	};

	[[nodiscard]] constexpr Outcome Written(bool a_changed) noexcept { return a_changed ? Outcome::kChanged : Outcome::kUnchanged; }

	using FieldWriter = Outcome (*)(RE::TESForm*, const std::string& a_text);

	// TESAmmo::shortDesc (AMMO ONAM), CommonLibF4 TESBoundObjects.h, offset 178.
	// Same field name FallHook 1.10.163 uses (RuntimeTextSimpleFields::ApplyAmmoShortDescription).
	Outcome WriteAmmoShortName(RE::TESForm* a_form, const std::string& a_text)
	{
		const auto ammo = a_form->As<RE::TESAmmo>();
		if (!ammo) {
			return Outcome::kWrongType;
		}
		return Written(TextWrite::Assign(ammo->shortDesc, a_text));
	}

	// AlchemyItem::Data::addictionName (ALCH DNAM), CommonLibF4 MagicItems.h, offset 1A8+20.
	Outcome WriteAddictionName(RE::TESForm* a_form, const std::string& a_text)
	{
		const auto alchemy = a_form->As<RE::AlchemyItem>();
		if (!alchemy) {
			return Outcome::kWrongType;
		}
		return Written(TextWrite::Assign(alchemy->data.addictionName, a_text));
	}

	// TESObjectDOOR::altOpenText (DOOR ONAM) / altCloseText (DOOR CNAM), CommonLibF4 TESBoundAnimObjects.h, offsets 108 / 110.
	// The ONAM/CNAM mapping follows xEdit ("Alternate text - Open/Close") and FallHook 1.10.163.
	Outcome WriteDoorOpenText(RE::TESForm* a_form, const std::string& a_text)
	{
		const auto door = a_form->As<RE::TESObjectDOOR>();
		if (!door) {
			return Outcome::kWrongType;
		}
		return Written(TextWrite::Assign(door->altOpenText, a_text));
	}

	Outcome WriteDoorCloseText(RE::TESForm* a_form, const std::string& a_text)
	{
		const auto door = a_form->As<RE::TESObjectDOOR>();
		if (!door) {
			return Outcome::kWrongType;
		}
		return Written(TextWrite::Assign(door->altCloseText, a_text));
	}

	// AVIF ANAM (abbreviation). The CommonLibF4 header puts it at 0x1A0, which is the 1.11 (AE/NG) layout.
	// On Fallout 4 1.10.163 the slot is at 0x160 (FallHook src/110163/RuntimeActorValueAnamFixer.cpp:
	// "AVIF ANAM offset 0x160", while its src/111191 variant uses 0x1A0). AE 0x1A0 is proven from the AE load
	// code in .191/.221/.240 (docs/phaseC-c3-misc-body.md row 5); CommonLibF4AE carries the OG value here.
	Outcome WriteAbbreviation(RE::TESForm* a_form, const std::string& a_text)
	{
		constexpr std::ptrdiff_t kOffsetOG = 0x160;
		constexpr std::ptrdiff_t kOffsetAE = 0x1A0;
		const auto               actorValue = a_form->As<RE::ActorValueInfo>();
		if (!actorValue) {
			return Outcome::kWrongType;
		}
		const auto offset = DSD::IsAE() ? kOffsetAE : kOffsetOG;
		auto&      slot = *reinterpret_cast<RE::BGSLocalizedString*>(reinterpret_cast<std::byte*>(actorValue) + offset);
		return Written(TextWrite::Assign(slot, a_text));
	}

	// INGR YNAM is not display text: the 4-byte value is the pickup sound slot of IngredientItem
	// (AE load code stores it at +0x158; ticket 14). Listed in core/HandledTypes.h so it does not show up as unknown.
	constexpr auto kIngredientYnam = DSD::TypeOf("INGR YNAM");

	struct Field
	{
		DSD::TypeCode type;
		FieldWriter   write;
	};

	constexpr std::array kFields{
		Field{ DSD::TypeOf("AMMO ONAM"), WriteAmmoShortName },
		Field{ DSD::TypeOf("ALCH DNAM"), WriteAddictionName },
		Field{ DSD::TypeOf("DOOR ONAM"), WriteDoorOpenText },
		Field{ DSD::TypeOf("DOOR CNAM"), WriteDoorCloseText },
		Field{ DSD::TypeOf("AVIF ANAM"), WriteAbbreviation },
	};
}

namespace MiscTypes
{
	void ApplyAll()
	{
		const auto& store = Manager::GetSingleton().Store();

		std::map<std::string, ApplyStats> stats;
		for (const auto& field : kFields) {
			auto& s = stats[DSD::TypeCodeToString(field.type)];
			store.ForEach(field.type, [&](std::uint32_t a_formID, std::uint32_t, const DSD::Entry& a_entry) {
				const auto form = DSD::LookupFormByID(a_formID);
				if (!form) {
					++s.formMissing;
					logger::debug("{} {:08X}: form not found", DSD::TypeCodeToString(field.type), a_formID);
					return;
				}
				switch (field.write(form, a_entry.text)) {
				case Outcome::kUnchanged:
					++s.unchanged;
					[[fallthrough]];
				case Outcome::kChanged:
					++s.applied;
					break;
				case Outcome::kWrongType:
					++s.wrongType;
					logger::debug("{} {:08X}: form type {} has no such field", DSD::TypeCodeToString(field.type), a_formID, form->GetFormTypeString());
					break;
				}
			});
		}

		store.ForEach(kIngredientYnam, [&](std::uint32_t, std::uint32_t, const DSD::Entry&) {
			++stats[DSD::TypeCodeToString(kIngredientYnam)].skipped;
		});

		logger::info("Misc single-field translations applied:");
		for (const auto& [name, s] : stats) {
			logger::info("  {} : {:>6} applied ({} unchanged), {:>4} form missing, {:>4} wrong type, {:>4} skipped (not display text)", name, s.applied, s.unchanged, s.formMissing, s.wrongType, s.skipped);
		}
	}
}
