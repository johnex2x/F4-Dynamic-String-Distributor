#include "Settings.h"

#include <SimpleIni.h>

void Settings::Load()
{
	const auto path = std::format("Data/F4SE/Plugins/{}.ini", Plugin::NAME);

	CSimpleIniA ini;
	ini.SetUnicode();
	if (ini.LoadFile(path.c_str()) < 0) {
		logger::info("{} not found, using defaults", path);
		return;
	}

	constexpr auto general = "General";
	enablePlayerRef = ini.GetBoolValue(general, "bEnablePlayerRef", enablePlayerRef);
	updateInterval = std::max(0.0f, static_cast<float>(ini.GetDoubleValue(general, "fUpdateInterval", updateInterval)));

	dialogueOptionHooks = ini.GetBoolValue(general, "bDialogueOptionHooks", dialogueOptionHooks);
	activateTextHooks = ini.GetBoolValue(general, "bActivateTextHooks", activateTextHooks);
	refreshInstanceNames = ini.GetBoolValue(general, "bRefreshInstanceNames", refreshInstanceNames);

	constexpr auto debug = "Debug";
	debugLog = ini.GetBoolValue(debug, "bEnableDebugLog", debugLog);
	debugInfo = ini.GetBoolValue(debug, "bEnableDebugInfo", debugInfo);
	diagnoseNames = ini.GetBoolValue(debug, "bDiagnoseNames", diagnoseNames);
	logNameRefresh = ini.GetBoolValue(debug, "bLogNameRefresh", logNameRefresh);
	nameRefreshDryRun = ini.GetBoolValue(debug, "bNameRefreshDryRun", nameRefreshDryRun);
	logOverrideNames = ini.GetBoolValue(debug, "bLogOverrideNames", logOverrideNames);
	xdiMovieCallback = ini.GetBoolValue(debug, "bXdiMovieCallback", xdiMovieCallback);
	xdiSetMemberHook = ini.GetBoolValue(debug, "bXdiSetMemberHook", xdiSetMemberHook);
}
