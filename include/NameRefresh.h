#pragma once

// One-shot repair of display names that placed references and the player's items cached before DSD wrote
// its translations (ExtraTextDisplayData built from the base name and the instance naming rules).
// Rebuilds only armor and weapon names that the game composes itself; custom names, quest text and
// items without instance data stay untouched. Other inventories are only counted.
// Writes one info line per call that starts with "[NameRefresh]". Detail lines need bLogNameRefresh.
namespace NameRefresh
{
	// Call after the const and misc re-apply at NewGame and PostLoadGame. a_tag only labels the log lines.
	void Apply(std::string_view a_tag);
}