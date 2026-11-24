#pragma once

#include "core/TranslationStore.h"

// Const fields ported from upstream SSE-DSD (ticket 13):
// NPC_ SHRT, LSCR DESC, MGEF DNAM, REFR FULL (map markers), REGN RDMP, PERK EPF2.
// ConstTranslation.cpp lists the writers in its indexed-field table and applies them with the others
// (PERK EPF2 has its own loop there, for "choice").
namespace PortedConstFields
{
	// Same meaning as ConstTranslation's Slot
	enum class Slot
	{
		kApplied,
		kWrongType,  // the form has no such field
		kNoSlot,     // the index points past the form's list
		kUnchanged   // the field already held the text (counts as applied)
	};

	Slot WriteLoadScreenText(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text);
	Slot WriteMagicDescription(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text);
	Slot WriteNpcShortName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text);
	Slot WriteMapMarkerName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text);
	Slot WriteRegionMapName(RE::TESForm* a_form, std::uint32_t a_index, const std::string& a_text);
	// a_choice: 1-based position among the perk's Activate choices (the entry's "choice"), 0 = every choice with the index
	Slot WritePerkActivateChoice(RE::TESForm* a_form, std::uint32_t a_index, std::uint32_t a_choice, const std::string& a_text);

	// Writes the NPC's short name from the store, if it has one. Called when the game copies an NPC's
	// name from a template, because the short name is copied along with it.
	void ApplyNpcShortName(RE::TESForm* a_npc);
}
