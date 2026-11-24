#pragma once

// Leveled item list override name (LVLI ONAM). The game copies it onto items generated from the list.
// The text lives in a global map with no known OG Address Library ID, so the reader is hooked instead.
namespace LeveledListNames
{
	// Builds the lookup from the store and, the first time rows exist, hooks TESLevItem::GetOverrideName.
	// Call when game data is ready, after the store is loaded.
	void Apply();
}