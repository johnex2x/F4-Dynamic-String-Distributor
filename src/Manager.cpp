#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"

namespace
{
	[[nodiscard]] std::vector<DSD::PluginInfo> BuildLoadOrder()
	{
		std::vector<DSD::PluginInfo> loadOrder;

		const auto handler = DSD::GetDataHandler();
		if (!handler) {
			return loadOrder;
		}

		for (const auto file : handler->files) {
			if (!file || file->GetCompileIndex() == 0xFF) {  // only active files
				continue;
			}
			loadOrder.push_back({ std::string{ file->GetFilename() },
				file->GetCompileIndex(),
				file->GetSmallFileCompileIndex(),
				file->IsLight() });
		}

		return loadOrder;
	}
}

void Manager::LoadTranslations()
{
	const auto start = std::chrono::steady_clock::now();

	const auto loadOrder = BuildLoadOrder();
	logger::info("Load order: {} active plugins", loadOrder.size());

	const bool debugInfo = Settings::GetSingleton().debugInfo;
	if (debugInfo) {
		logger::info("bEnableDebugInfo: texts get a \"FormID|Plugin|\" prefix");
	}

	const auto stats = _store.Load(DSD_PATH, loadOrder, debugInfo);
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

	LogStats(stats, elapsed);
}

bool Manager::EnsureLoaded()
{
	if (_loaded.load(std::memory_order_acquire)) {
		return true;
	}

	std::scoped_lock lock{ _loadMutex };
	if (_loaded.load(std::memory_order_relaxed)) {
		return true;
	}
	if (BuildLoadOrder().empty()) {
		return false;
	}

	LoadTranslations();
	_loaded.store(true, std::memory_order_release);
	return true;
}

void Manager::Reload()
{
	std::scoped_lock lock{ _loadMutex };
	if (BuildLoadOrder().empty()) {
		logger::warn("Reload: no plugins loaded, keeping the old translations");
		return;
	}
	logger::info("Reload: game data was reloaded, re-reading the translation files");
	LoadTranslations();
	_loaded.store(true, std::memory_order_release);
}

void Manager::LogStats(const DSD::LoadStats& a_stats, std::chrono::milliseconds a_elapsed) const
{
	logger::info("Translations: {} files parsed, {} failed, {} entries read, {} stored in {} ms",
		a_stats.filesParsed, a_stats.filesFailed, a_stats.entriesRead, _store.Size(), a_elapsed.count());

	if (a_stats.entriesBadFormID || a_stats.entriesBadType || a_stats.entriesPluginNotLoaded) {
		logger::warn("Skipped entries: {} bad form_id, {} bad type, {} plugin not loaded",
			a_stats.entriesBadFormID, a_stats.entriesBadType, a_stats.entriesPluginNotLoaded);
	}

	for (const auto& folder : a_stats.ignoredFolders) {
		logger::debug("Ignored folder {} (plugin not in load order)", folder);
	}

	for (const auto& error : a_stats.errors) {
		logger::error("{}", error);
	}

	std::vector<std::pair<std::string, DSD::LoadStats::TypeStats>> types;
	types.reserve(a_stats.types.size());
	for (const auto& [type, typeStats] : a_stats.types) {
		types.emplace_back(DSD::TypeCodeToString(type), typeStats);
	}
	std::ranges::sort(types, {}, &decltype(types)::value_type::first);

	std::size_t unhandledTypes = 0;
	std::size_t unhandledEntries = 0;
	for (const auto& [name, typeStats] : types) {
		const bool handled = DSD::IsHandledType(*DSD::MakeTypeCode(name));
		if (!handled) {
			++unhandledTypes;
			unhandledEntries += typeStats.stored;
		}
		logger::info("  {} : {:>6} stored, {:>4} shadowed{}", name, typeStats.stored, typeStats.shadowed, handled ? "" : "  [not handled yet]");
	}

	if (unhandledTypes) {
		logger::warn("{} types ({} entries) are not handled yet", unhandledTypes, unhandledEntries);
	}
}
