#pragma once

// Fallout 4 single-field text stored in forms: AMMO ONAM, ALCH DNAM, DOOR ONAM/CNAM, AVIF ANAM
namespace MiscTypes
{
	// Writes the translations and logs per-type results (call when game data is ready and after loading a save)
	void ApplyAll();
}
