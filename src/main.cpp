#include "ActivateText.h"
#include "BodyPartsChargen.h"
#include "ConstTranslation.h"
#include "Dialogue.h"
#include "DialogueOptions.h"
#include "LeveledListNames.h"
#include "Manager.h"
#include "MiscTypes.h"
#include "NameDiag.h"
#include "NameRefresh.h"
#include "RuntimeText.h"
#include "Runtime.h"
#include "Settings.h"

namespace
{
	// Set when F4SE reports that the game unloads its data (Mods menu); the next GameDataReady reloads
	bool g_dataUnloaded = false;

	// Times each phase of a message handler and logs one summary line, so a slow phase is easy to spot
	class PhaseTimer
	{
	public:
		template <class F>
		void Run(std::string_view a_name, F&& a_phase)
		{
			const auto start = std::chrono::steady_clock::now();
			a_phase();
			const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
			_total += ms;
			_line += std::format("{}{} {:.1f}", _line.empty() ? "" : ", ", a_name, ms);
		}

		void Log(std::string_view a_title) const
		{
			logger::info("{} timings (ms): {}; total {:.1f}", a_title, _line, _total);
		}

	private:
		std::string _line;
		double      _total{ 0.0 };
	};

	void MessageListener(F4SE::MessagingInterface::Message* a_message)
	{
		const auto& features = DSD::GetFeatures();
		if (!features.base) {
			return;
		}
		switch (a_message->type) {
		case F4SE::MessagingInterface::kPostLoad:
			logger::info("PostLoad");
			if (features.dialogueOptions) {
				DialogueOptions::InstallXdi();
			}
			break;
		case F4SE::MessagingInterface::kGameDataReady:
			// F4SE passes "is ready" as the data pointer value
			// and sends it with false when the Mods menu unloads the data before a reload
			if (!a_message->data) {
				logger::info("GameDataReady(false): game data is being unloaded");
				if (features.runtimeText) {
					RuntimeText::Reset();
				}
				if (features.dialogueOptions) {
					DialogueOptions::Reset();
				}
				if (features.bodyParts) {
					BodyPartsChargen::Reset();
				}
				g_dataUnloaded = true;
			} else {
				logger::info("GameDataReady");
				const bool diagnose = features.nameDiag && Settings::GetSingleton().diagnoseNames;
				if (diagnose) {
					NameDiag::WalkInteriors("GDRstart");
				}
				PhaseTimer timer;
				timer.Run("translations", [] {
					if (std::exchange(g_dataUnloaded, false)) {
						Manager::GetSingleton().Reload();
					} else {
						Manager::GetSingleton().EnsureLoaded();
					}
				});
				if (features.constTranslation) {
					timer.Run("const", [] { ConstTranslation::ApplyAll(); });
				}
				if (features.miscTypes) {
					timer.Run("misc", [] { MiscTypes::ApplyAll(); });
				}
				if (features.bodyParts) {
					timer.Run("bodyparts", [] { BodyPartsChargen::Apply(); });
				}
				if (features.prompts) {
					timer.Run("prompts", [] { Dialogue::ApplyPrompts(); });
				}
				if (features.runtimeText) {
					timer.Run("runtime", [] { RuntimeText::Apply(); });
				}
				if (features.dialogueOptions) {
					timer.Run("options", [] { DialogueOptions::Apply(); });
				}
				if (features.activateText) {
					timer.Run("activate", [] { ActivateText::Apply(); });
				}
				if (features.leveledNames) {
					timer.Run("levelednames", [] { LeveledListNames::Apply(); });
				}
				timer.Log("GameDataReady");
				if (diagnose) {
					NameDiag::Install();
					NameDiag::ArmoCensus("GDR");
					NameDiag::WalkInteriors("GDRend");
				}
			}
			spdlog::default_logger()->flush();  // trace lines are not flushed one by one
			break;
		case F4SE::MessagingInterface::kPostLoadGame:
		case F4SE::MessagingInterface::kNewGame:
			if (!features.constTranslation && !features.miscTypes && !features.bodyParts && !features.nameRefresh && !features.nameDiag) {
				break;  // AE: none of the re-applied features is on
			}
			// The ReconstructForms hook upstream uses has no known OG ID; re-apply names here instead
			logger::info("{}: re-applying const translations", a_message->type == F4SE::MessagingInterface::kNewGame ? "NewGame" : "PostLoadGame");
			if (features.nameDiag && Settings::GetSingleton().diagnoseNames) {
				NameDiag::ArmoCensus("before");
				NameDiag::WalkLoaded("before");
				NameDiag::WalkInteriors("before");
			}
			{
				PhaseTimer timer;
				if (features.constTranslation) {
					timer.Run("const", [] { ConstTranslation::ApplyAll(); });
				}
				if (features.miscTypes) {
					timer.Run("misc", [] { MiscTypes::ApplyAll(); });
				}
				if (features.bodyParts) {
					timer.Run("bodyparts", [] { BodyPartsChargen::Apply(); });
				}
				timer.Log(a_message->type == F4SE::MessagingInterface::kNewGame ? "NewGame" : "PostLoadGame");
			}
			if (features.nameRefresh && Settings::GetSingleton().refreshInstanceNames) {
				NameRefresh::Apply(a_message->type == F4SE::MessagingInterface::kNewGame ? "NewGame" : "PostLoadGame");
			}
			if (features.nameDiag && Settings::GetSingleton().diagnoseNames) {
				NameDiag::ArmoCensus("after");
				NameDiag::WalkLoaded("after");
			}
			spdlog::default_logger()->flush();
			break;
		case F4SE::MessagingInterface::kPreSaveGame:
			if (features.nameDiag && Settings::GetSingleton().diagnoseNames) {
				NameDiag::ArmoCensus("presave");
				NameDiag::WalkLoaded("presave");
				spdlog::default_logger()->flush();
			}
			break;
		default:
			break;
		}
	}
}

F4SE_EXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data{};
	data.PluginName(Plugin::NAME);
	data.PluginVersion(Plugin::VERSION);
	data.AuthorName("SkyHorizon (original), F4 port"sv);
	// OG (1.10.163) and AE (1.11.x) only; NG (1.10.98x) is not supported.
	// F4SE 0.7.x on 1.11.x (PluginManager.cpp, 0.7.9) admits a plugin without a version list
	// only when both bit 2 flags are set: kAddressIndependence_AddressLibrary_1_11_137 and
	// kStructureIndependence_1_11_137Layout. Bit 1 (the 1.10.980 flags) stays clear, so NG F4SE
	// refuses the DLL. The fork has no setters for bit 2 yet.
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

	if (a_f4se->IsEditor()) {
		return false;
	}

	const auto runtime = a_f4se->RuntimeVersion();
	if (DSD::ClassifyRuntime(runtime) == DSD::Runtime::Unsupported) {
		return false;
	}

	return true;
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se)
{
	if (a_f4se->IsEditor() || DSD::ClassifyRuntime(a_f4se->RuntimeVersion()) == DSD::Runtime::Unsupported) {
		return false;
	}
	F4SE::Init(a_f4se);
	// From here on a missing Address Library ID on 1.11.x aborts the game with a message instead of resolving to a
	// wrong function. The fork's own load-time globals ran before this line (cmake/patches/commonlibf4-runtime-detection.patch).
	REL::IDDB::SetStrictLookup(true);

	auto& settings = Settings::GetSingleton();
	settings.Load();

	const auto level = settings.debugLog ? spdlog::level::trace : spdlog::level::info;
	spdlog::set_level(level);
	// Flushing every trace line writes to disk on the game thread (trace output can reach tens of thousands of lines).
	// Flush on info and above, and when the process ends; the handlers above also flush after their work.
	spdlog::flush_on(spdlog::level::info);
	std::atexit([] { spdlog::shutdown(); });

	logger::info("{} v{} loaded", Plugin::NAME, Plugin::VERSION.string());
	logger::info("Game version: {}", a_f4se->RuntimeVersion().string());
	logger::info("F4SE version: {}", a_f4se->F4SEVersion().string());

	const auto& features = DSD::GetFeatures();
	if (!features.base) {
		logger::warn("Game version {}: no DSD feature is enabled for this runtime; no hooks or game-data listener installed.", a_f4se->RuntimeVersion().string());
		return true;
	}
	if (DSD::GetRuntime() == DSD::Runtime::AE) {
		// Every feature off here has an unproven chain (an ID, a struct member or a hook signature); see docs/runtime-support.md
		// and docs/phaseC-ae-id-audit.md
		logger::info("AE runtime: features OFF (chain not proven from the AE executables): {}", DSD::DescribeDisabledFeatures(features));
	}

	F4SE::AllocTrampoline(1 << 10);  // 1 << 9 was enough for the older hooks; the dialogue option hooks add three more

	if (features.prompts) {
		Dialogue::Install();
	}
	if (features.constTranslation) {
		ConstTranslation::InstallHooks();
	}
	if (features.runtimeText) {
		RuntimeText::Install();
	}
	if (features.dialogueOptions) {
		DialogueOptions::Install();
	}
	if (features.activateText) {
		ActivateText::Install();
	}

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageListener)) {
		logger::critical("Failed to register messaging listener");
		return false;
	}

	return true;
}
