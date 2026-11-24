#pragma once

#include "core/HandledTypes.h"

// Writes translations into form fields that the game keeps in memory (names, game settings, ...).
// Runs once the game data is ready, and again whenever the game rebuilds forms.
namespace ConstTranslation
{
	// Applies every stored const translation and logs per-type results
	void ApplyAll();

	// Installs the hook that writes an NPC's name before the game copies it to another NPC
	void InstallHooks();

	// Type code helpers live in core/HandledTypes.h
	using DSD::IsBaseNameType;
	using DSD::Record;
	using DSD::Signature;
	using DSD::Subrecord;

	// FULL index 0/1 is the form's own name; index > 1 are object template names (ticket 11)
	[[nodiscard]] constexpr bool IsBaseNameIndex(std::uint32_t a_index) noexcept { return a_index <= 1; }
}
