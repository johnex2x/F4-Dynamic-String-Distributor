#pragma once

// Activate prompt text (ACTI/FLOR/FURN ATTX). The game keeps it in a global map, not in the form,
// so it is replaced on the HUD rollover instead of being written to the form.
namespace ActivateText
{
	// Installs the hooks (call at F4SE load)
	void Install();

	// Builds the lookup table and logs the result (call once when game data is ready)
	void Apply();
}
