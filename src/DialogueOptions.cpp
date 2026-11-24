#include "DialogueOptions.h"

#include "AeHookBytes.h"
#include "HookUtil.h"
#include "Manager.h"
#include "Runtime.h"
#include "Settings.h"
#include "core/DialogueOptions.h"

// Player dialogue options (ticket 06, second pass).
//
// Why hooks: Dialogue::ApplyPrompts patches the game's prompt map once at GameDataReady, but about 7,700 of the
// translated INFO RNAM prompts are not in that map yet (it fills lazily). XDI also reads that map directly and,
// for an INFO without RNAM, shows the INFO's response text (INFO NAM1), which is only translated when the line
// is spoken. So the text is replaced when the menu shows it, by INFO identity (form IDs), never by comparing
// the visible text (the game's strings are Korean, the JSON `original` is English).
//
// Two paths, both ported from FallHook 1.10.163 (GPL-3.0, https://github.com/Agent69-stack/FallHook):
//
//  1. Vanilla wheel (src/110163/RuntimeDialogueButtonHook.cpp, RuntimeDialogueChoiceContext.cpp,
//     RuntimeDialogueChoiceTranslations.cpp):
//       BGSSceneActionPlayerDialogue::GetCurrentTopicInfo, OG ID 781358, remembers the INFO per button;
//       DialogueMenuUtils::SetButtonText, OG ID 643840, replaces the button text.
//     Source: INFO RNAM, then DIAL FULL.
//
//  2. XDI (RuntimeXdiDialogueMenuHook.cpp, RuntimeXdiDialogueOptionResolver.cpp,
//     RuntimeXdiDialogueOptionTranslations.cpp, RuntimeXdiDialogueInfoResolver.cpp):
//       XDI registers GetDialogueOptions on DialogueMenu.swf's BGSCodeObj. The movie is found through an F4SE
//       Scaleform callback; the function is wrapped (now, or late through a hook on Scaleform
//       Value::ObjectInterface::SetMember, OG ID 1360149, when XDI registers after us). The wrapper calls XDI's
//       function and rewrites the "prompt" and "response" members of each returned option object.
//     Source: prompt = INFO RNAM; response = INFO NAM1.
//     Scaleform calls and their OG IDs come from the CommonLibF4 fork (GFx_Player.h); GetElement, OG ID 827659,
//     is not in the fork and comes from FallHook (RuntimeXdiDialogueMenuHook.cpp, kGetElementID).
//
// Hook method: prologue hooks (HookUtil::InstallPrologueHook), as FallHook does for the same IDs. A prologue the
// decoder does not support is refused with a log line and that hook stays off.
namespace
{
	namespace GFx = RE::Scaleform::GFx;
	using Value = GFx::Value;
	using ObjectInterface = GFx::Value::ObjectInterface;

	constexpr std::size_t        kButtonCount = 4;          // FallHook kButtonCount: response types 0..3
	constexpr std::size_t        kContextsPerButton = 4;    // FallHook kContextsPerButton
	constexpr std::size_t        kMaxChain = 8;             // dataInfo chain depth kept per captured INFO
	constexpr std::uint32_t      kContextTtlMs = 1500;      // FallHook kContextTtl: older captures are dropped
	constexpr std::uint32_t      kTraceLimit = 192;         // trace lines per data load (FallHook kTraceLimit)
	constexpr std::uint32_t      kNoOption = 0xFFFFFFFFu;

	// XDI's optionID order for the vanilla four-option layout (FallHook kVanillaOrder)
	constexpr std::array<std::uint32_t, kButtonCount> kVanillaOrder{ 3, 0, 1, 2 };

	constexpr char kDialogueMenuPath[] = "Interface/DialogueMenu.swf";  // FallHook kDialogueMenuPath
	constexpr char kCodeObjPath[] = "root.Menu_mc.BGSCodeObj";           // FallHook kCodeObjPath
	constexpr char kGetOptionsName[] = "GetDialogueOptions";
	constexpr char kOriginalName[] = "__DSD_OriginalGetDialogueOptions";
	constexpr char kWrapperName[] = "__DSD_WrapperGetDialogueOptions";
	constexpr char kMarkerName[] = "__DSD_Target";

	// Set between Apply and Reset; hooks do nothing otherwise (they read the translation store)
	std::atomic_bool g_active{ false };

	// True only after a DialogueMenu was found and XDI has not registered GetDialogueOptions yet.
	// The SetMember hook ignores every call while it is false.
	std::atomic_bool g_awaitingXdi{ false };

	std::atomic_uint32_t g_traceLines{ 0 };
	// One info line the first time each outcome happens, so the log shows the path working without bEnableDebugLog
	std::atomic_bool g_firstWheelHit{ false };
	std::atomic_bool g_firstWheelMiss{ false };
	std::atomic_bool g_firstXdiHit{ false };
	std::atomic_bool g_firstXdiMiss{ false };

	[[nodiscard]] bool TraceEnabled()
	{
		return Settings::GetSingleton().debugLog;
	}

	// Cap: the log must not flood while the player talks
	[[nodiscard]] bool TakeTraceSlot()
	{
		return TraceEnabled() && g_traceLines.load(std::memory_order_relaxed) < kTraceLimit && g_traceLines.fetch_add(1, std::memory_order_relaxed) < kTraceLimit;
	}

	// a_message builds the text, so nothing is formatted after the first time
	template <class F>
	void LogFirst(std::atomic_bool& a_flag, F&& a_message)
	{
		if (!a_flag.load(std::memory_order_relaxed) && !a_flag.exchange(true, std::memory_order_relaxed)) {
			logger::info("Dialogue options: first {}", a_message());
		}
	}

	// ---- captured INFO chains (vanilla wheel, and XDI when it shows the vanilla four-option layout) ----

	struct Chain
	{
		std::array<DSD::InfoLink, kMaxChain> links{};
		std::size_t                          count{ 0 };
		std::uint32_t                        topicFormID{ 0 };

		[[nodiscard]] std::span<const DSD::InfoLink> Span() const noexcept { return { links.data(), count }; }
	};

	// Follows TESTopicInfo::dataInfo like the DialogueResponse hook's FindResponseOwner (Dialogue.cpp):
	// an INFO that owns responses is the NAM1 owner, a shared-response INFO has none and points on.
	[[nodiscard]] Chain MakeChain(RE::TESTopicInfo* a_info)
	{
		Chain chain;
		for (auto info = a_info; info && chain.count < kMaxChain; info = info->dataInfo) {
			const auto head = info->responses.head;
			chain.links[chain.count++] = DSD::InfoLink{
				.formID = info->GetFormID(),
				.responseID = head ? head->data.responseID : std::uint32_t{ 0 },
				.hasResponses = head != nullptr,
			};
		}
		if (a_info && a_info->parentTopic) {
			chain.topicFormID = a_info->parentTopic->GetFormID();
		}
		return chain;
	}

	struct Captured
	{
		Chain         chain;
		std::uint64_t tickMs{ 0 };
	};

	std::mutex                                                          g_contextLock;
	std::array<std::array<Captured, kContextsPerButton>, kButtonCount> g_contexts;  // newest first
	std::uint64_t                                                       g_lastContextMs{ 0 };

	void RememberContext(std::uint32_t a_button, RE::TESTopicInfo* a_info)
	{
		if (a_button >= kButtonCount || !a_info) {
			return;
		}
		auto chain = MakeChain(a_info);
		if (chain.count == 0) {
			return;
		}

		const auto       now = ::GetTickCount64();
		std::scoped_lock lock{ g_contextLock };
		if (g_lastContextMs == 0 || now - g_lastContextMs > kContextTtlMs) {
			g_contexts = {};
		}
		g_lastContextMs = now;
		auto& slots = g_contexts[a_button];
		std::move_backward(slots.begin(), slots.end() - 1, slots.end());
		slots.front() = Captured{ chain, now };
	}

	// Visits the captures of one button, newest first, under the lock (no copy). a_visit returns true to stop.
	template <class F>
	void VisitContexts(std::uint32_t a_button, F&& a_visit)
	{
		if (a_button >= kButtonCount) {
			return;
		}
		std::scoped_lock lock{ g_contextLock };
		for (const auto& captured : g_contexts[a_button]) {
			if (captured.chain.count != 0 && a_visit(captured)) {
				return;
			}
		}
	}

	void ClearContexts()
	{
		std::scoped_lock lock{ g_contextLock };
		g_contexts = {};
		g_lastContextMs = 0;
	}

	// ---- vanilla wheel ----

	// BGSSceneActionPlayerDialogue::GetCurrentTopicInfo(action, scene, target, responseType), OG Address Library ID 781358
	// (ID and signature from FallHook src/110163/RuntimeDialogueButtonHook.cpp, kGetCurrentTopicInfoID).
	// responseType 0..3 is the button the INFO is shown on.
	struct GetCurrentTopicInfo
	{
		static RE::TESTopicInfo* thunk(RE::BGSSceneActionPlayerDialogue* a_action, RE::BGSScene* a_scene, RE::TESObjectREFR* a_target, std::uint32_t a_responseType)
		{
			const auto result = func(a_action, a_scene, a_target, a_responseType);
			if (g_active.load(std::memory_order_acquire)) {
				RememberContext(a_responseType, result);
			}
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	// DialogueMenuUtils::SetButtonText(button, text, challengeLevel, isEnabled), OG Address Library ID 643840
	// (ID and signature from FallHook src/110163/RuntimeDialogueButtonHook.cpp, kSetButtonTextID).
	struct SetButtonText
	{
		static void thunk(std::uint8_t a_button, RE::BSStringT<char>& a_text, std::uint8_t a_challengeLevel, bool a_isEnabled)
		{
			if (a_button < kButtonCount && !a_text.empty() && g_active.load(std::memory_order_acquire)) {
				Replace(a_button, a_text);
			}
			func(a_button, a_text, a_challengeLevel, a_isEnabled);
		}
		static inline REL::Relocation<decltype(thunk)> func;

	private:
		static void Replace(std::uint8_t a_button, RE::BSStringT<char>& a_text)
		{
			const auto&   store = Manager::GetSingleton().Store();
			bool          anyContext = false;
			bool          replaced = false;
			std::uint32_t firstInfo = 0;
			std::uint32_t firstTopic = 0;

			VisitContexts(a_button, [&](const Captured& a_captured) {
				const auto& chain = a_captured.chain;
				if (!anyContext) {
					anyContext = true;
					firstInfo = chain.links[0].formID;
					firstTopic = chain.topicFormID;
				}
				const auto hit = DSD::ResolveWheelOption(store, chain.Span(), chain.topicFormID);
				if (!hit || hit.entry->text.empty() || hit.entry->text.size() >= std::numeric_limits<std::uint16_t>::max()) {
					return false;
				}
				DSD::BSStringSet(a_text, hit.entry->text.c_str(), hit.entry->text.size() + 1);
				replaced = true;
				if (TakeTraceSlot()) {
					logger::trace("Wheel option {}: INFO {:08X} topic {:08X} -> {} {:08X}", a_button, chain.links[0].formID, chain.topicFormID, DSD::OptionSourceName(hit.source), hit.formID);
				}
				LogFirst(g_firstWheelHit, [&] { return std::format("wheel option replaced (button {}, {}, INFO {:08X})", a_button, DSD::OptionSourceName(hit.source), chain.links[0].formID); });
				return true;
			});
			if (replaced) {
				return;
			}

			// No translation for any captured INFO of this button, or the game never called GetCurrentTopicInfo for it
			if (TakeTraceSlot()) {
				if (anyContext) {
					logger::trace("Wheel option {}: miss, INFO {:08X} topic {:08X}", a_button, firstInfo, firstTopic);
				} else {
					logger::trace("Wheel option {}: miss, no captured INFO", a_button);
				}
			}
			LogFirst(g_firstWheelMiss, [&] { return std::format("wheel option left as is (button {}, {})", a_button, anyContext ? "no translation" : "no INFO captured"); });
		}
	};

	// ---- XDI ----

	// Value and ObjectInterface pieces the fork keeps private: object interface at 0x00, data pointer at 0x10
	// (GFx_Player.h Value members; static_assert guards the size).
	struct ValueLayout
	{
		ObjectInterface* objectInterface;
		std::int32_t     type;
		std::int32_t     pad;
		void*            data;
		std::size_t      dataAux;
	};
	static_assert(sizeof(ValueLayout) == sizeof(Value));

	// Value::ObjectInterface::GetElement(data, index, out): OG Address Library ID 827659 (ID and signature from FallHook
	// src/110163/RuntimeXdiDialogueMenuHook.cpp, kGetElementID/GetElementFunc), AE ID 2285881 (docs/phaseD-dialogue-options.md).
	// The runtime is fixed for the whole run, so the ID is chosen once.
	[[nodiscard]] bool GetElement(const Value& a_array, std::uint32_t a_index, Value& a_out)
	{
		const auto& raw = reinterpret_cast<const ValueLayout&>(a_array);
		if (!raw.objectInterface || !raw.data) {
			return false;
		}
		static const REL::Relocation<bool(ObjectInterface*, void*, std::uint32_t, Value*)> func{ DSD::IsAE() ? REL::ID(2285881) : REL::ID(827659) };
		return func(raw.objectInterface, raw.data, a_index, std::addressof(a_out));
	}

	// Value::ObjectInterface::GetMember(data, name, out, isDisplayObject): OG ID 1517430 (the fork's), AE ID 4494126.
	// The fork's own AE ID for it (2285936) is not in the AE Address Library and would abort the game, so every GetMember
	// of this file goes through here (docs/phaseD-dialogue-options.md).
	[[nodiscard]] bool GetMemberRaw(ObjectInterface* a_interface, void* a_data, const char* a_name, Value* a_out, bool a_isDisplayObject)
	{
		static const REL::Relocation<bool(ObjectInterface*, void*, const char*, Value*, bool)> func{ DSD::IsAE() ? REL::ID(4494126) : REL::ID(1517430) };
		return func(a_interface, a_data, a_name, a_out, a_isDisplayObject);
	}

	[[nodiscard]] bool GetMember(const Value& a_object, const char* a_name, Value* a_out)
	{
		const auto& raw = reinterpret_cast<const ValueLayout&>(a_object);
		return raw.objectInterface && a_object.IsObject() && GetMemberRaw(raw.objectInterface, raw.data, a_name, a_out, a_object.IsDisplayObject());
	}

	// AE: GetCurrentTopicInfo hook installed (XDI reads the captured INFOs, so it needs it); set by Install
	bool g_aeContextHook = false;

	// Set while this plugin itself sets GetDialogueOptions, so the SetMember hook does not wrap its own write
	thread_local bool g_internalSetMember = false;

	// Serializes WrapNow and LateWrap. The game can create the DialogueMenu movie on two threads at once (AE log
	// 2026-11-22: "wrapped" twice, 0.13 s apart, threads 18872 and 2768), and both passed the "already wrapped" check
	// before either wrote. Scaleform objects are not safe to change from two threads. Only taken on the rare wrap
	// path, never on the SetMember fast path. WrapNow sets g_internalSetMember before its writes, so the SetMember
	// hook never calls LateWrap on a thread that already holds this lock.
	std::mutex g_wrapLock;

	struct ScopedInternalSetMember
	{
		ScopedInternalSetMember() { g_internalSetMember = true; }
		~ScopedInternalSetMember() { g_internalSetMember = false; }
	};

	[[nodiscard]] std::optional<std::uint32_t> OptionID(const Value& a_option)
	{
		Value value;
		if (!GetMember(a_option, "optionID", std::addressof(value))) {
			return std::nullopt;
		}
		if (value.IsUInt()) {
			return value.GetUInt();
		}
		if (value.IsInt() && value.GetInt() >= 0) {
			return static_cast<std::uint32_t>(value.GetInt());
		}
		if (value.IsNumber() && value.GetNumber() >= 0.0 && value.GetNumber() < 4294967295.0) {
			return static_cast<std::uint32_t>(value.GetNumber());
		}
		return std::nullopt;
	}

	// The vanilla layout lists the options in kVanillaOrder (3, 0, 1, 2), possibly with gaps
	[[nodiscard]] bool IsVanillaOptionLayout(const Value& a_result, std::uint32_t a_count)
	{
		if (a_count == 0 || a_count > kVanillaOrder.size()) {
			return false;
		}
		std::size_t orderIndex = 0;
		for (std::uint32_t i = 0; i < a_count; ++i) {
			Value option;
			if (!GetElement(a_result, i, option) || !option.IsObject()) {
				return false;
			}
			const auto id = OptionID(option);
			if (!id) {
				return false;
			}
			while (orderIndex < kVanillaOrder.size() && kVanillaOrder[orderIndex] != *id) {
				++orderIndex;
			}
			if (orderIndex == kVanillaOrder.size()) {
				return false;
			}
			++orderIndex;
		}
		return true;
	}

	// Option k of a non-vanilla layout is the k-th INFO (with responses or a dataInfo) of the active player dialogue
	// action's response topics (FallHook RuntimeXdiDialogueInfoResolver.cpp). Player singleton: DSD::GetPlayer (OG ID 303410 in the fork, AE ID 2698073).
	[[nodiscard]] std::vector<RE::TESTopicInfo*> CollectActiveInfos()
	{
		std::vector<RE::TESTopicInfo*> infos;

		const auto player = DSD::GetPlayer();
		const auto scene = player ? player->GetCurrentScene() : nullptr;
		if (!scene) {
			return infos;
		}

		RE::BGSSceneActionPlayerDialogue* dialogue = nullptr;
		for (const auto action : scene->actions) {
			if (!action || action->GetActionType() != RE::BGSSceneAction::kPlayerDialogue) {
				continue;
			}
			const auto phase = scene->currentActivePhase;
			if (action->status.get() == RE::BGSSceneAction::Status::kRunning || (action->startPhase <= phase && action->endPhase >= phase)) {
				dialogue = static_cast<RE::BGSSceneActionPlayerDialogue*>(action);
				break;
			}
		}
		if (!dialogue) {
			return infos;
		}

		infos.reserve(16);
		for (const auto topic : dialogue->responseTopics) {
			if (!topic || !topic->topicInfos) {
				continue;
			}
			for (std::uint32_t i = 0; i < topic->numTopicInfos; ++i) {
				const auto info = topic->topicInfos[i];
				if (info && (info->responses.head || info->dataInfo) && std::ranges::find(infos, info) == infos.end()) {
					infos.push_back(info);
				}
			}
		}
		return infos;
	}

	struct XdiResolved
	{
		DSD::XdiOption option;
		const char*    how{ "none" };
		std::uint32_t  infoFormID{ 0 };
	};

	// Vanilla layout: the INFOs captured for button optionID (GetCurrentTopicInfo). The newest capture is the one the
	// game just asked for, but several may be listed, so an INFO with an RNAM translation wins over a newer one without.
	// A response (NAM1) alone is only taken from the newest capture, so a stale INFO cannot put its line on this option.
	[[nodiscard]] XdiResolved ResolveCaptured(const DSD::TranslationStore& a_store, std::uint32_t a_optionID)
	{
		XdiResolved resolved;
		bool        newest = true;
		VisitContexts(a_optionID, [&](const Captured& a_captured) {
			const auto option = DSD::ResolveXdiOption(a_store, a_captured.chain.Span());
			if (option.prompt) {
				resolved = { option, "captured", a_captured.chain.links[0].formID };
				return true;
			}
			if (newest && option.response) {
				resolved = { option, "captured", a_captured.chain.links[0].formID };
			}
			newest = false;
			return false;
		});
		return resolved;
	}

	void PatchOptions(Value& a_result)
	{
		if (!a_result.IsArray()) {
			return;
		}
		const auto count = a_result.GetArraySize();
		const auto vanilla = IsVanillaOptionLayout(a_result, count);

		// The scene is only walked when needed (non-vanilla layout)
		std::vector<RE::TESTopicInfo*> active;
		if (!vanilla) {
			active = CollectActiveInfos();
		}
		const auto& store = Manager::GetSingleton().Store();

		for (std::uint32_t i = 0; i < count; ++i) {
			Value option;
			if (!GetElement(a_result, i, option) || !option.IsObject()) {
				continue;
			}
			const auto id = OptionID(option);
			const auto optionValue = id.value_or(kNoOption);

			XdiResolved resolved;
			// One lookup per option: the active scene for other layouts, the captured INFOs for the vanilla layout
			if (id && !vanilla) {
				if (*id < active.size()) {
					resolved.option = DSD::ResolveXdiOption(store, MakeChain(active[*id]).Span());
					resolved.how = "active";
					resolved.infoFormID = active[*id]->GetFormID();
				}
			} else if (id) {
				resolved = ResolveCaptured(store, *id);
			}

			if (!resolved.option) {
				if (TakeTraceSlot()) {
					logger::trace("XDI option {} (optionID {}): miss, layout {}, INFO {:08X}", i, optionValue, vanilla ? "vanilla" : "other", resolved.infoFormID);
				}
				LogFirst(g_firstXdiMiss, [&] { return std::format("XDI option left as is (optionID {}, layout {})", optionValue, vanilla ? "vanilla" : "other"); });
				continue;
			}

			const auto& prompt = resolved.option.prompt;
			const auto& response = resolved.option.response;
			if (prompt && !prompt.entry->text.empty()) {
				option.SetMember("prompt", Value{ prompt.entry->text.c_str() });
			}
			if (response && !response.entry->text.empty()) {
				option.SetMember("response", Value{ response.entry->text.c_str() });
			}
			if (TakeTraceSlot()) {
				logger::trace("XDI option {} (optionID {}, {}): INFO {:08X} prompt {} {:08X}, response {} {:08X}", i, optionValue, resolved.how, resolved.infoFormID,
					prompt ? DSD::OptionSourceName(prompt.source) : "-", prompt.formID, response ? DSD::OptionSourceName(response.source) : "-", response.formID);
			}
			LogFirst(g_firstXdiHit, [&] {
				return std::format("XDI option replaced (optionID {}, {}, prompt {}, response {})", optionValue, resolved.how,
					prompt ? DSD::OptionSourceName(prompt.source) : "-", response ? DSD::OptionSourceName(response.source) : "-");
			});
		}
	}

	// Stands in for XDI's GetDialogueOptions: runs the original, then rewrites the option texts
	class XdiGetDialogueOptionsHandler final : public GFx::FunctionHandler
	{
	public:
		// The Scaleform heap operators of the base class are not used: this object lives as long as the game
		static void* operator new(std::size_t a_size) { return ::operator new(a_size); }
		static void  operator delete(void* a_ptr) noexcept { ::operator delete(a_ptr); }
		static void  operator delete(void* a_ptr, std::size_t) noexcept { ::operator delete(a_ptr); }

		void Call(const Params& a_params) override
		{
			if (!a_params.self || !a_params.retVal || !a_params.self->Invoke(kOriginalName, a_params.retVal, a_params.args, a_params.argCount)) {
				if (a_params.retVal) {
					*a_params.retVal = Value{};
				}
				return;
			}
			if (g_active.load(std::memory_order_acquire)) {
				PatchOptions(*a_params.retVal);
			}
		}
	};

	// Value::ObjectInterface::SetMember(data, name, value, isDisplayObject), OG Address Library ID 1360149
	// (ID and signature from CommonLibF4 GFx_Player.h and FallHook RuntimeXdiDialogueMenuHook.cpp, kSetMemberID).
	// Watches for XDI registering GetDialogueOptions after our wrapper is armed.
	struct SetMemberHook
	{
		static bool thunk(ObjectInterface* a_this, void* a_data, const char* a_name, const Value& a_value, bool a_isDisplayObject)
		{
			// Fast path for every Scaleform member write in the game: one relaxed load, then straight to the original.
			// The name is only looked at while a DialogueMenu waits for XDI to register GetDialogueOptions.
			if (!g_awaitingXdi.load(std::memory_order_relaxed)) {
				return func(a_this, a_data, a_name, a_value, a_isDisplayObject);
			}
			const auto result = func(a_this, a_data, a_name, a_value, a_isDisplayObject);
			if (result && a_name && a_name[0] == 'G' && !g_internalSetMember && std::strcmp(a_name, kGetOptionsName) == 0) {
				LateWrap(a_this, a_data, a_value, a_isDisplayObject);
			}
			return result;
		}
		static inline REL::Relocation<decltype(thunk)> func;

	private:
		static void LateWrap(ObjectInterface* a_this, void* a_data, const Value& a_original, bool a_isDisplayObject)
		{
			std::scoped_lock lock{ g_wrapLock };
			if (!g_awaitingXdi.load(std::memory_order_relaxed)) {
				return;  // another thread wrapped it while this one waited for the lock
			}
			Value marker;
			if (!GetMemberRaw(a_this, a_data, kMarkerName, std::addressof(marker), a_isDisplayObject) || !marker.IsBoolean() || !marker.GetBoolean()) {
				return;
			}
			Value wrapper;
			if (!GetMemberRaw(a_this, a_data, kWrapperName, std::addressof(wrapper), a_isDisplayObject) || wrapper.IsUndefined()) {
				return;
			}
			// func writes straight through, so this does not re-enter the hook
			func(a_this, a_data, kOriginalName, a_original, a_isDisplayObject);
			func(a_this, a_data, kGetOptionsName, wrapper, a_isDisplayObject);
			g_awaitingXdi.store(false, std::memory_order_relaxed);
			logger::info("Dialogue options: XDI GetDialogueOptions wrapped (late)");
		}
	};

	[[nodiscard]] bool IsDialogueMenu(GFx::Movie* a_movie)
	{
		Value path;
		return a_movie && a_movie->GetVariable(std::addressof(path), "root.loaderInfo.url") && path.IsString() &&
		       std::strcmp(path.GetString(), kDialogueMenuPath) == 0;
	}

	void WrapNow(Value& a_codeObj, GFx::Movie* a_movie)
	{
		std::scoped_lock lock{ g_wrapLock };
		{
			Value existing;
			if (GetMember(a_codeObj, kOriginalName, std::addressof(existing)) && !existing.IsUndefined()) {
				return;  // already wrapped (the menu movie can be created again)
			}
		}

		// Owned by the game's refcount from here on; one reference is intentionally kept so it is never freed
		Value wrapper;
		a_movie->CreateFunction(std::addressof(wrapper), new XdiGetDialogueOptionsHandler());

		ScopedInternalSetMember guard;
		a_codeObj.SetMember(kMarkerName, Value{ true });
		a_codeObj.SetMember(kWrapperName, wrapper);

		Value original;
		if (GetMember(a_codeObj, kGetOptionsName, std::addressof(original)) && !original.IsUndefined()) {
			a_codeObj.SetMember(kOriginalName, original);
			a_codeObj.SetMember(kGetOptionsName, wrapper);
			g_awaitingXdi.store(false, std::memory_order_relaxed);
			logger::info("Dialogue options: XDI GetDialogueOptions wrapped");
		} else {
			// XDI has not registered yet; SetMemberHook wraps it when it does
			g_awaitingXdi.store(true, std::memory_order_relaxed);
			logger::info("Dialogue options: DialogueMenu found, waiting for XDI GetDialogueOptions");
		}
	}

	// F4SE Scaleform callback: runs for every movie the game creates
	bool F4SEAPI RegisterScaleform(GFx::Movie* a_movie, Value*)
	{
		if (!IsDialogueMenu(a_movie)) {
			return true;
		}
		Value codeObj;
		if (!a_movie->GetVariable(std::addressof(codeObj), kCodeObjPath) || !codeObj.IsObject()) {
			logger::warn("Dialogue options: DialogueMenu has no {}; XDI options stay untranslated", kCodeObjPath);
			return true;
		}
		WrapNow(codeObj, a_movie);
		return true;
	}
}

namespace DialogueOptions
{
	void Install()
	{
		if (!Settings::GetSingleton().dialogueOptionHooks) {
			logger::info("bDialogueOptionHooks = false: dialogue option hooks not installed");
			return;
		}

		if (DSD::IsAE()) {
			// AE: IDs and bytes in docs/phaseD-dialogue-options.md. SetButtonText writes through BSStringT::Set, so its
			// code is checked first. The XDI part waits for kPostLoad (InstallXdi): XDI.dll may load after this plugin.
			bool context = false;
			bool button = false;
			const REL::Relocation<std::uintptr_t> set{ REL::ID(4471693) };
			if (HookUtil::CodeMatches(set.address(), AeHookBytes::kBSStringSet, "BSStringT::Set")) {
				context = HookUtil::InstallPrologueHookChecked<GetCurrentTopicInfo>(2196825, "BGSSceneActionPlayerDialogue::GetCurrentTopicInfo", AeHookBytes::kGetCurrentTopicInfo);
				button = HookUtil::InstallPrologueHookChecked<SetButtonText>(2249662, "DialogueMenuUtils::SetButtonText", AeHookBytes::kSetButtonText);
			}
			logger::info("Dialogue options: vanilla wheel hooks {}", context && button ? "installed" : "NOT fully installed, wheel options stay untranslated");
			g_aeContextHook = context;
			return;
		}

		const auto context = HookUtil::InstallPrologueHook<GetCurrentTopicInfo>(781358, "BGSSceneActionPlayerDialogue::GetCurrentTopicInfo");
		const auto button = HookUtil::InstallPrologueHook<SetButtonText>(643840, "DialogueMenuUtils::SetButtonText");
		logger::info("Dialogue options: vanilla wheel hooks {}", context && button ? "installed" : "NOT fully installed, wheel options stay untranslated");

		// The captured-INFO hook is useless without the button hook and the other way round, but XDI uses the captured
		// INFOs too (vanilla layout), so GetCurrentTopicInfo stays on if only SetButtonText failed.
		if (!HookUtil::InstallPrologueHook<SetMemberHook>(1360149, "Scaleform::GFx::Value::ObjectInterface::SetMember")) {
			logger::info("Dialogue options: XDI hook NOT installed, XDI options stay untranslated");
			return;
		}

		const auto scaleform = F4SE::GetScaleformInterface();
		if (!scaleform || !scaleform->Register("DynamicStringDistributorXDI", RegisterScaleform)) {
			logger::error("Dialogue options: could not register the DialogueMenu Scaleform callback, XDI options stay untranslated");
			return;
		}
		logger::info("Dialogue options: XDI wrapper registered for {}", kDialogueMenuPath);
	}

	void InstallXdi()
	{
		if (!DSD::IsAE() || !Settings::GetSingleton().dialogueOptionHooks) {
			return;
		}
		// Called at kPostLoad: F4SE has loaded every plugin by then, so the answer cannot change later. The SetMember hook sits
		// on every Scaleform member write in the game, so it is only installed when XDI exists.
		if (!GetModuleHandleW(L"XDI.dll")) {
			logger::info("Dialogue options: XDI.dll is not loaded, XDI path not installed");
			return;
		}
		if (!g_aeContextHook) {
			logger::info("Dialogue options: GetCurrentTopicInfo hook is not installed, XDI path not installed");
			return;
		}
		const auto& settings = Settings::GetSingleton();
		logger::info("Dialogue options: XDI crash test switches: bXdiSetMemberHook = {}, bXdiMovieCallback = {}", settings.xdiSetMemberHook, settings.xdiMovieCallback);
		if (settings.xdiSetMemberHook) {
			if (!HookUtil::InstallPrologueHookChecked<SetMemberHook>(2286589, "Scaleform::GFx::Value::ObjectInterface::SetMember", AeHookBytes::kGFxSetMember)) {
				logger::info("Dialogue options: XDI hook NOT installed, XDI options stay untranslated");
				return;
			}
		} else {
			logger::info("Dialogue options: SetMember hook skipped (bXdiSetMemberHook = false)");
		}
		if (!settings.xdiMovieCallback) {
			logger::info("Dialogue options: Scaleform movie callback skipped (bXdiMovieCallback = false)");
			return;
		}
		const auto scaleform = F4SE::GetScaleformInterface();
		if (!scaleform || !scaleform->Register("DynamicStringDistributorXDI", RegisterScaleform)) {
			logger::error("Dialogue options: could not register the DialogueMenu Scaleform callback, XDI options stay untranslated");
			return;
		}
		logger::info("Dialogue options: XDI wrapper registered for {}", kDialogueMenuPath);
	}

	void Apply()
	{
		g_traceLines.store(0, std::memory_order_relaxed);
		g_firstWheelHit.store(false, std::memory_order_relaxed);
		g_firstWheelMiss.store(false, std::memory_order_relaxed);
		g_firstXdiHit.store(false, std::memory_order_relaxed);
		g_firstXdiMiss.store(false, std::memory_order_relaxed);
		ClearContexts();
		g_active.store(true, std::memory_order_release);

		if (Settings::GetSingleton().dialogueOptionHooks) {
			logger::info("Dialogue options: ready");
		}
	}

	void Reset()
	{
		g_active.store(false, std::memory_order_release);
		ClearContexts();
	}
}
