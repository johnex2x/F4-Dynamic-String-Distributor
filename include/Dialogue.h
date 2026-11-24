#pragma once

// Dialogue text: spoken lines (INFO NAM1) and player prompts (INFO RNAM)
namespace Dialogue
{
	// Installs the response hook (call at F4SE load)
	void Install();

	// Writes INFO RNAM translations into the game's prompt table (call when game data is ready).
	// The vanilla dialogue menu and XDI both read prompts from this table.
	void ApplyPrompts();
}
