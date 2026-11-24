#include "MenuRefCountFix.h"

// A standalone fix, kept apart from DynamicStringDistributor: it patches other plugins' code, not game text.
// It uses no Address Library ID and installs no hook; removing the DLL turns the fix off.
namespace
{
	// OG 1.10.163 and AE 1.11.x, the same runtimes as DynamicStringDistributor
	[[nodiscard]] constexpr bool IsSupported(const REL::Version& a_version) noexcept
	{
		return a_version == REL::Version{ 1, 10, 163, 0 } || (a_version[0] == 1 && a_version[1] == 11);
	}

	void MessageListener(F4SE::MessagingInterface::Message* a_message)
	{
		if (a_message->type == F4SE::MessagingInterface::kPostLoad) {
			MenuRefCountFix::Apply();  // every plugin is loaded now; nothing renders yet
			spdlog::default_logger()->flush();
		}
	}
}

F4SE_EXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data{};
	data.PluginName(Plugin::NAME);
	data.PluginVersion(Plugin::VERSION);
	data.AuthorName("DSD F4 port"sv);
	// F4SE 0.7.x on 1.11.x admits a plugin without a version list only when both bit 2 flags are set
	// (kAddressIndependence_AddressLibrary_1_11_137, kStructureIndependence_1_11_137Layout). Bit 1 stays clear.
	data.addressIndependence = 1u << 2;
	data.structureIndependence = 1u << 2;
	return data;
}();

// OG F4SE (1.10.163) loads plugins through Query + Load
F4SE_EXPORT bool F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION.pack();
	return !a_f4se->IsEditor() && IsSupported(a_f4se->RuntimeVersion());
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	if (a_f4se->IsEditor() || !IsSupported(a_f4se->RuntimeVersion())) {
		return false;
	}
	F4SE::Init(a_f4se);
	spdlog::set_level(spdlog::level::info);
	spdlog::flush_on(spdlog::level::info);

	logger::info("{} v{} loaded, game {}", Plugin::NAME, Plugin::VERSION.string(), a_f4se->RuntimeVersion().string());

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageListener)) {
		logger::critical("Failed to register messaging listener");
		return false;
	}
	return true;
}
