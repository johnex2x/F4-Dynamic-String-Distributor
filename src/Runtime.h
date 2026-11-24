#pragma once

namespace DSD
{
	// Supported runtime families: OG 1.10.163 and AE 1.11.x.
	// NG (1.10.980 / 1.10.984) is not supported.
	// AE is a family, not a version list: each hook validates its own target bytes.
	enum class Runtime { OG, AE, Unsupported };

	[[nodiscard]] constexpr Runtime ClassifyRuntime(const REL::Version& a_version) noexcept
	{
		if (a_version == REL::Version{ 1, 10, 163, 0 }) {
			return Runtime::OG;
		}
		if (a_version[0] == 1 && a_version[1] == 11) {
			return Runtime::AE;
		}
		return Runtime::Unsupported;
	}

	[[nodiscard]] inline Runtime GetRuntime() noexcept
	{
		return ClassifyRuntime(REL::Module::get().version());
	}

	// Per-feature gates. On OG every feature is on and behaves exactly as before.
	// On AE a feature is on only when its whole chain (Address Library IDs, hook bytes, every struct member it
	// touches) is proven from the AE executables; see docs/ae-crosscheck.md and docs/runtime-support.md.
	// The gates are read at install and apply time only, never inside a hook.
	struct Features
	{
		bool base{ false };              // load order (TESDataHandler.files, TESFile), form lookup, translation store
		bool activateText{ false };      // ACTI/FLOR/FURN ATTX: GetActivateText + ShowRollover hooks
		bool leveledNames{ false };      // LVLI ONAM: TESLevItem GetOverrideName vtable hook
		bool constTranslation{ false };  // FULL/DESC/MESG/QUST/TERM/GMST/... writers and the FULL CopyComponent hook
		bool miscTypes{ false };         // AMMO/ALCH/DOOR/AVIF texts
		bool bodyParts{ false };         // BPTD/RACE chargen names
		bool prompts{ false };           // dialogue prompts (INFO RNAM) and the spoken response constructor hook
		bool runtimeText{ false };       // description and quest journal readers
		bool dialogueOptions{ false };   // dialogue wheel / XDI option text
		bool nameRefresh{ false };       // instance name repair
		bool nameDiag{ false };          // name diagnostics
	};

	[[nodiscard]] inline Features MakeFeatures(Runtime a_runtime) noexcept
	{
		switch (a_runtime) {
		case Runtime::OG:
			return { true, true, true, true, true, true, true, true, true, true, true };
		case Runtime::AE:
			// Chains proven from AE executables (.191/.221/.240), see docs/ae-crosscheck.md "Phase B proofs" and
			// docs/phaseC-ae-id-audit.md (every ID an enabled path reaches is checked in all six 1.11.x bins).
			// dialogueOptions: vanilla wheel plus the XDI path (docs/phaseD-dialogue-options.md); XDI hooks only when
			// XDI.dll is loaded (DialogueOptions::InstallXdi at kPostLoad). nameRefresh: walk layout and engine calls proven in
			// docs/phaseE-namerefresh-proof.md, engine bytes checked before each run. Still off: nameDiag (its cell-attach event
			// source is not analyzed). constTranslation includes the
			// sparse FULL map (STAT/MSTT/SCOL/KYWD/FLST/LIGH names): ON after a one-time layout guard in ConstTranslation.cpp
			// (docs/phaseD-sparse-fullname.md); if the guard fails only that group is off.
			return { .base = true,
				.activateText = true,
				.leveledNames = true,
				.constTranslation = true,
				.miscTypes = true,
				.bodyParts = true,
				.prompts = true,
				.runtimeText = true,
				.dialogueOptions = true,
				.nameRefresh = true };
		default:
			return {};
		}
	}

	// True on AE. Resolved once; for apply-time and install-time decisions, never inside a hook
	[[nodiscard]] inline bool IsAE() noexcept
	{
		static const bool ae = GetRuntime() == Runtime::AE;
		return ae;
	}

	// Computed once (first call is at plugin load)
	[[nodiscard]] inline const Features& GetFeatures() noexcept
	{
		static const Features features = MakeFeatures(GetRuntime());
		return features;
	}

	// One line for the log: the features that are off on this runtime
	[[nodiscard]] inline std::string DescribeDisabledFeatures(const Features& a_features)
	{
		std::string out;
		const auto  add = [&](bool a_on, std::string_view a_name) {
			if (!a_on) {
				out += std::format("{}{}", out.empty() ? "" : ", ", a_name);
			}
		};
		add(a_features.base, "base");
		add(a_features.activateText, "activateText");
		add(a_features.leveledNames, "leveledNames");
		add(a_features.constTranslation, "constTranslation");
		add(a_features.miscTypes, "miscTypes");
		add(a_features.bodyParts, "bodyParts");
		add(a_features.prompts, "prompts");
		add(a_features.runtimeText, "runtimeText");
		add(a_features.dialogueOptions, "dialogueOptions");
		add(a_features.nameRefresh, "nameRefresh");
		add(a_features.nameDiag, "nameDiag");
		return out;
	}

	// ---- Game access that differs between runtimes -------------------------------------------------------------
	// CommonLibF4 (alandtse fork) ships wrong 1.11.x IDs for its form map, form lock and TESDataHandler singleton
	// (its second ID slot is the 1.10.98x ID space) and a missing ID silently resolves to the next one in the bin,
	// so enabled AE features must never call those wrappers. These two helpers are the only path to them.

	// TESDataHandler singleton. AE: ID 4796135 (docs/ae-crosscheck.md, Phase B proofs).
	[[nodiscard]] inline RE::TESDataHandler* GetDataHandler()
	{
		static const bool ae = GetRuntime() == Runtime::AE;
		if (ae) {
			static const REL::Relocation<RE::TESDataHandler**> singleton{ REL::ID(4796135) };
			return *singleton;
		}
		return RE::TESDataHandler::GetSingleton();
	}

	// Form lookup by ID. AE: the engine's own lookup function (ID 2193092: read lock, hash map, unlock), so no
	// hash-map layout is assumed.
	[[nodiscard]] inline RE::TESForm* LookupFormByID(std::uint32_t a_formID)
	{
		static const bool ae = GetRuntime() == Runtime::AE;
		if (ae) {
			static const REL::Relocation<RE::TESForm*(std::uint32_t)> lookup{ REL::ID(2193092) };
			return lookup(a_formID);
		}
		return RE::TESForm::GetFormByID(a_formID);
	}

	// Game heap allocation (RE::malloc). The fork's MemoryManager singleton ID (2193197) is absent on 1.11.x.
	// AE: the singleton getter is ID 4471522 (a function with a local-static guard, returns MemoryManager&);
	// Allocate (ID 2267872) is the same ID in the fork's second slot and in CommonLibF4AE.
	[[nodiscard]] inline void* GameMalloc(std::size_t a_size)
	{
		if (IsAE()) {
			static const REL::Relocation<RE::MemoryManager&()> singleton{ REL::ID(4471522) };
			return singleton().Allocate(a_size, 0, false);
		}
		return RE::malloc(a_size);
	}

	// PlayerCharacter singleton: the fork's AE ID (2690919) is not in the AE Address Library, so AE uses ID 2698073
	// (docs/phaseD-dialogue-options.md). Read after game data is ready, as the fork's GetSingleton is.
	[[nodiscard]] inline RE::PlayerCharacter* GetPlayer()
	{
		if (IsAE()) {
			static const REL::Relocation<RE::NiPointer<RE::PlayerCharacter>*> singleton{ REL::ID(2698073) };
			return singleton->get();
		}
		return RE::PlayerCharacter::GetSingleton();
	}

	// BSStringT<char>::Set(str, maxLen). The fork's ID (2189084) is absent on 1.11.x; AE: ID 4471693
	// (docs/phaseC-c4-runtimetext-prompts.md 1c). The caller must have checked the code bytes (AeHookBytes::kBSStringSet).
	inline void BSStringSet(RE::BSStringT<char>& a_string, const char* a_text, std::uint64_t a_maxLen)
	{
		if (IsAE()) {
			static const REL::Relocation<void(RE::BSStringT<char>*, const char*, std::uint64_t)> set{ REL::ID(4471693) };
			set(std::addressof(a_string), a_text, a_maxLen);
			return;
		}
		a_string.Set(a_text, a_maxLen);
	}

	// GameSettingCollection singleton. The fork's ID (2690301) is absent on 1.11.x; AE: data ID 4797590
	// (a GameSettingCollection*). Checked against the class vtable (ID 618060, the RTTI vtable in all three exes),
	// so a wrong object is never used; call it from apply code, not from a hook.
	[[nodiscard]] inline RE::GameSettingCollection* GetGameSettingCollection()
	{
		if (IsAE()) {
			static const REL::Relocation<RE::GameSettingCollection**> singleton{ REL::ID(4797590) };
			static const REL::Relocation<std::uintptr_t>              vtable{ REL::ID(618060) };
			const auto collection = *singleton;
			if (collection && *reinterpret_cast<const std::uintptr_t*>(collection) != vtable.address()) {
				logger::error("GameSettingCollection singleton does not hold a GameSettingCollection (vtable {:X}); game settings are not translated",
					*reinterpret_cast<const std::uintptr_t*>(collection));
				return nullptr;
			}
			return collection;
		}
		return RE::GameSettingCollection::GetSingleton();
	}
}
