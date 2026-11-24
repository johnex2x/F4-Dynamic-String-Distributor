#pragma once

#include "core/HandledTypes.h"

// Owns the translation store inside the game and reports what was loaded
class Manager
{
public:
	[[nodiscard]] static Manager& GetSingleton() noexcept
	{
		static Manager singleton;
		return singleton;
	}

	static constexpr auto DSD_PATH = "Data/F4SE/Plugins/DynamicStringDistributor";

	// Reads all translation files for the current load order and logs statistics
	void LoadTranslations();

	// Loads the translations the first time it is called while plugins are loaded.
	// Hooks that run during data load call it, so names exist before NPCs copy them.
	// Returns false while the game has no plugins yet.
	bool EnsureLoaded();

	// Re-reads the translation files after the game reloaded its data (Mods menu).
	// Call only while no hook reads the store, that is between F4SE's two GameDataReady messages.
	void Reload();

	[[nodiscard]] const DSD::TranslationStore& Store() const noexcept { return _store; }

private:
	Manager() = default;

	void LogStats(const DSD::LoadStats& a_stats, std::chrono::milliseconds a_elapsed) const;

	DSD::TranslationStore                     _store;
	std::atomic_bool                          _loaded{ false };
	std::mutex                                _loadMutex;
};
