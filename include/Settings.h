#pragma once

// Values from Data/F4SE/Plugins/DynamicStringDistributor.ini (same keys as upstream DSD)
class Settings
{
public:
	[[nodiscard]] static Settings& GetSingleton() noexcept
	{
		static Settings singleton;
		return singleton;
	}

	void Load();

	bool  enablePlayerRef{ false };
	float updateInterval{ 5.0f };  // seconds, scaled by game time; <= 0 disables updates
	bool  debugLog{ false };
	bool  debugInfo{ false };
	bool  diagnoseNames{ false };  // on: read-only [NameDiag] log lines about display names that stay Korean
	bool  refreshInstanceNames{ true };  // off: leave the display names that placed references and the player's items cached at load
	bool  logNameRefresh{ false };  // on: [NameRefresh] detail lines (the summary line is always written)
	bool  nameRefreshDryRun{ false };  // on: [NameRefresh] walks and counts but writes no name (first test on a new runtime)
	bool  logOverrideNames{ false };  // on: one [LeveledListNames] line per leveled list name the game reads
	bool  dialogueOptionHooks{ true };  // off: no dialogue option (wheel and XDI) hooks
	bool  activateTextHooks{ true };  // off: no crosshair prompt hooks (in case another HUD mod conflicts)
	bool  xdiMovieCallback{ true };   // AE crash test: off = no F4SE Scaleform movie callback for XDI
	bool  xdiSetMemberHook{ true };   // AE crash test: off = no GFx SetMember hook for XDI

private:
	Settings() = default;
};
