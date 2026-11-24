#pragma once

// VATS body part names (BPTD BPTN) and character creation option names
// (RACE TTGP tint groups and options, MPPN face presets, FMRN face regions)
namespace BodyPartsChargen
{
	// Writes the translations into the loaded forms and logs per-type results.
	// Safe to run again (after a save is loaded): slots that already hold the translation count as applied.
	void Apply();

	// Drops what Apply read from the plugin files (call when the game unloads its data, before a reload)
	void Reset();
}
