#pragma once

// Read-only diagnostic for display names that stay Korean on references and inventory items although
// the base form's name is translated. Off unless the ini key bDiagnoseNames is set; it changes nothing
// in the game and only writes info lines that start with "[NameDiag]".
namespace NameDiag
{
	// Registers the cell attach sink (once). Call after the game data is ready.
	void Install();

	// Counts references (loaded cells) and player inventory stacks whose display text holds Hangul
	void WalkLoaded(std::string_view a_tag);

	// Same counts over every interior cell whose references are in memory; also lists display texts that differ from the base name
	void WalkInteriors(std::string_view a_tag);

	// Counts ARMO forms whose own name holds Hangul
	void ArmoCensus(std::string_view a_tag);
}
