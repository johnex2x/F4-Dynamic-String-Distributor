#pragma once

// The one place that decides which type codes the plugin handles (writes into a form, or hooks at runtime).
// The game modules keep their own dispatch tables; when a module gains or loses a type, update IsHandledType too.
// Game-independent, so a test can list the types of Fallout4.json that nothing handles.

#include "core/TranslationStore.h"

#include <algorithm>
#include <array>

namespace DSD
{
	[[nodiscard]] constexpr std::uint32_t Signature(const char (&a_sig)[5]) noexcept
	{
		return (static_cast<std::uint32_t>(a_sig[0]) << 24) | (static_cast<std::uint32_t>(a_sig[1]) << 16) |
		       (static_cast<std::uint32_t>(a_sig[2]) << 8) | static_cast<std::uint32_t>(a_sig[3]);
	}

	// Record and subrecord signatures of a type code, e.g. 'ARMO' and 'FULL'
	[[nodiscard]] constexpr std::uint32_t Record(TypeCode a_type) noexcept { return static_cast<std::uint32_t>(a_type >> 32); }
	[[nodiscard]] constexpr std::uint32_t Subrecord(TypeCode a_type) noexcept { return static_cast<std::uint32_t>(a_type & 0xFFFFFFFF); }

	// Name types written into TESFullName (ConstTranslation). REFR FULL (map markers) is a separate indexed field.
	// DIAL FULL is included: TESTopic derives from TESFullName.
	[[nodiscard]] constexpr bool IsBaseNameType(TypeCode a_type) noexcept
	{
		return Subrecord(a_type) == Signature("FULL") && Record(a_type) != Signature("REFR");
	}

	// Descriptions shown through TESDescription::GetDescription (RuntimeText). LSCR DESC is a const field (PortedConstFields).
	[[nodiscard]] constexpr bool IsDescriptionType(TypeCode a_type) noexcept
	{
		return Subrecord(a_type) == Signature("DESC") && Record(a_type) != Signature("LSCR");
	}

	inline constexpr auto kHandledTypes = std::to_array<TypeCode>({
		// ConstTranslation: game settings
		TypeOf("GMST DATA"),
		// ConstTranslation / PortedConstFields: indexed text fields (REFR FULL is not a base name)
		TypeOf("QUST NNAM"),
		TypeOf("MESG ITXT"),
		TypeOf("MESG NNAM"),
		TypeOf("TERM BTXT"),
		TypeOf("TERM ITXT"),
		TypeOf("TERM RNAM"),
		TypeOf("TERM UNAM"),
		TypeOf("TERM WNAM"),
		TypeOf("TERM NAM0"),
		TypeOf("NPC_ SHRT"),
		TypeOf("LSCR DESC"),
		TypeOf("MGEF DNAM"),
		TypeOf("REFR FULL"),
		TypeOf("REGN RDMP"),
		TypeOf("PERK EPF2"),
		// TemplateNames
		TypeOf("INNR WNAM"),
		// Dialogue
		TypeOf("INFO NAM1"),
		TypeOf("INFO RNAM"),
		// RuntimeText (descriptions are IsDescriptionType)
		TypeOf("QUST CNAM"),
		// MiscTypes (INGR YNAM is not display text: counted as skipped)
		TypeOf("AMMO ONAM"),
		TypeOf("ALCH DNAM"),
		TypeOf("DOOR ONAM"),
		TypeOf("DOOR CNAM"),
		TypeOf("AVIF ANAM"),
		TypeOf("INGR YNAM"),
		// BodyPartsChargen
		TypeOf("BPTD BPTN"),
		TypeOf("RACE TTGP"),
		TypeOf("RACE MPPN"),
		TypeOf("RACE FMRN"),
		// LeveledListNames
		TypeOf("LVLI ONAM"),
		// ActivateText
		TypeOf("ACTI ATTX"),
		TypeOf("FLOR ATTX"),
		TypeOf("FURN ATTX"),
	});

	[[nodiscard]] constexpr bool IsHandledType(TypeCode a_type) noexcept
	{
		return IsBaseNameType(a_type) || IsDescriptionType(a_type) || std::ranges::find(kHandledTypes, a_type) != kHandledTypes.end();
	}
}
