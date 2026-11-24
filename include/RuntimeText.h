#pragma once

// Text the game reads from the plugin files each time it is shown:
// descriptions (XXXX DESC) and quest journal entries (QUST CNAM)
namespace RuntimeText
{
	// Installs the hooks (call at F4SE load)
	void Install();

	// Builds the lookup tables from the loaded translations (call when game data is ready)
	void Apply();

	// Drops the lookup tables (they point into the translation store) before the store is reloaded
	void Reset();
}
