#pragma once

// Player dialogue options shown in the menu: the vanilla wheel and XDI (Extended Dialogue Interface).
// Both read the option text lazily from game state that is not complete at GameDataReady, so the text is
// replaced when the menu shows it. Rules (which source wins) live in core/DialogueOptions.h.
namespace DialogueOptions
{
	// Installs the hooks unless bDialogueOptionHooks is false (call at F4SE load)
	void Install();

	// AE only: installs the XDI part when XDI.dll is loaded (call at kPostLoad, when every F4SE plugin is loaded)
	void InstallXdi();

	// Enables the hooks for the loaded translations (call when game data is ready)
	void Apply();

	// Disables the hooks and forgets captured dialogue state (they read the translation store); call before it is reloaded
	void Reset();
}
