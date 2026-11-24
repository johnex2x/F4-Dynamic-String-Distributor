# Phase C: AE reachability audit of Address Library IDs

Scope: every `REL::ID`, `REL::RelocationID`, `RE::VTABLE` and `RE::RTTI` value that an AE-enabled DSD path can reach, directly or through the
CommonLibF4 (alandtse fork) headers, after the phase C changes. AE-enabled features: base, activateText, leveledNames, constTranslation
(without the sparse FULL map group), miscTypes, bodyParts, prompts, runtimeText (`src/Runtime.h`, `MakeFeatures`).
On 1.11.x a missing ID aborts the game (`cmake/patches/commonlibf4-runtime-detection.patch`), so one missing ID on an enabled path is a blocker.

Scripts (under `phaseC/impl/`, they reuse `ae/lib.py`): `audit_ids.py` (exact membership in the six bins 137/159/169/191/221/240
plus extra proofs; output `audit.out`), `scan_dll.py` and `trace_dll.py` (find absent IDs in the linked DLL and the functions holding them; output `trace.txt`).

## 1. Result

- 26 IDs checked, 0 missing (exact membership in all six 1.11.x bins).
- One blocker found and fixed: six fork globals look up absent IDs while the DLL loads (section 3).
- Every other absent ID linked into the DLL sits behind an OG-only branch or in a feature that is OFF on AE (section 4).

## 2. IDs reached by AE paths

Fork wrappers whose second slot is the 1.10.98x space are listed only where that second value is itself an exact bin member
(checked here; function identity shown in phase B/C docs). Not reached on AE: OG hook IDs (1510422, 755245, 523613, 139753, 1077221, 414044),
OG REGN Find 657534, the sparse FULL map, and the dialogueOptions / nameRefresh / nameDiag IDs.

| ID | kind | what | reach site | features | in 6 bins | RVA 191 / 221 / 240 |
|---|---|---|---|---|---|---|
| 4796135 | data | TESDataHandler singleton | src/Runtime.h:115 (GetDataHandler) | base, constTranslation (INNR) | yes (6/6) | 0x30dc000 / 0x30dc080 / 0x30e7100 |
| 2193092 | func | engine form lookup by ID | src/Runtime.h:127 (LookupFormByID) | all form-writing features | yes (6/6) | 0x311850 / 0x311860 / 0x311b80 |
| 4471522 | func | MemoryManager singleton getter | src/Runtime.h:139 (GameMalloc) | constTranslation (GMST) | yes (6/6) | 0x1dff80 / 0x1dff80 / 0x1e02a0 |
| 2267872 | func | MemoryManager::Allocate (fork 2nd slot) | extern/.../MemoryManager.h:207 via src/Runtime.h:140 | constTranslation (GMST) | yes (6/6) | 0x16579c0 / 0x1657ac0 / 0x1657de0 |
| 4797590 | data | GameSettingCollection* singleton | src/Runtime.h:151 (GetGameSettingCollection) | constTranslation (GMST) | yes (6/6) | 0x30ed820 / 0x30ed8a0 / 0x30f8920 |
| 618060 | vtbl | GameSettingCollection vtable (guard) | src/Runtime.h:152 | constTranslation (GMST) | yes (6/6) | 0x245fa88 / 0x245fa88 / 0x2467ae8 |
| 2196228 | func | TESRegionDataList::Find | src/PortedConstFields.cpp:121 | constTranslation (REGN RDMP) | yes (6/6) | 0x3ed3a0 / 0x3ed3b0 / 0x3ed6d0 |
| 846981 | vtbl | BGSEntryPointPerkEntry vtable | src/PortedConstFields.cpp:154 | constTranslation (PERK EPF2) | yes (6/6) | 0x24ca3d8 / 0x24ca3e8 / 0x24d2448 |
| 859152 | vtbl | BGSEntryPointFunctionDataActivateChoice vtable | src/PortedConstFields.cpp:155 | constTranslation (PERK EPF2) | yes (6/6) | 0x24c8688 / 0x24c8698 / 0x24d06f8 |
| 487988 | vtbl | TESFullName vtable | src/ConstTranslation.cpp:456 | constTranslation (hook) | yes (6/6) | 0x24625e0 / 0x24625e0 / 0x246a640 |
| 1206734 | vtbl | TESNPC vtable [5] (TESFullName part) | src/ConstTranslation.cpp:458 | constTranslation (hook) | yes (6/6) | 0x24ce808 / 0x24ce818 / 0x24d6878 |
| 2193213 | func | TESFullName::CopyComponent (hook) | src/ConstTranslation.cpp:457,469 | constTranslation (hook) | yes (6/6) | 0x315b90 / 0x315ba0 / 0x315ec0 |
| 2193019 | func | TESDescription::GetDescription (hook) | src/RuntimeText.cpp:181 | runtimeText | yes (6/6) | 0x30e6e0 / 0x30e6f0 / 0x30ea10 |
| 2207825 | func | TESQuest journal text reader (hook) | src/RuntimeText.cpp:182 | runtimeText | yes (6/6) | 0x672590 / 0x6725b0 / 0x6728d0 |
| 4471693 | func | BSStringT<char>::Set | src/RuntimeText.cpp:176 | runtimeText | yes (6/6) | 0x1e61d0 / 0x1e61d0 / 0x1e64f0 |
| 2227131 | func | DialogueResponse ctor (hook) | src/Dialogue.cpp:179 | prompts | yes (6/6) | 0xbae830 / 0xbae6a0 / 0xbae620 |
| 2662549 | data | INFO RNAM prompt table (BSTHashMap) | src/Dialogue.cpp:119 | prompts | yes (6/6) | 0x2f00408 / 0x2f00458 / 0x2f0b458 |
| 2193103 | func | TESForm::GetFile (fork 2nd slot) | extern/.../TESForms.h:751 via RuntimeText.cpp:84, BodyPartsChargen.cpp:56 | runtimeText, bodyParts | yes (6/6) | 0x3120a0 / 0x3120b0 / 0x3123d0 |
| 1309967 | data | form type name table (fork REL::ID) | extern/.../TESForms.h:826 via GetFormTypeString (debug log lines) | constTranslation, miscTypes, bodyParts, runtimeText | yes (6/6) | 0x2ed0b60 / 0x2ed0b60 / 0x2edbb60 |
| 2268729 | func | BSStringPool GetEntry<char> (fork 2nd slot) | extern/.../BSStringPool.h:142 via BSFixedString / BGSLocalizedString (TextWrite::Assign, response, prompt) | all text writers | yes (6/6) | 0x167d900 / 0x167da00 / 0x167dd20 |
| 2268720 | func | BSStringPool::Entry::release (fork 2nd slot) | extern/.../BSStringPool.h:21 | all text writers | yes (6/6) | 0x167d080 / 0x167d180 / 0x167d4a0 |
| 2267897 | func | BSReadWriteLock::lock_read (fork 2nd slot) | extern/.../BSLock.h:75 via ExtraDataList::GetByType (PortedConstFields.cpp:85) | constTranslation (REFR FULL) | yes (6/6) | 0x1658fe0 / 0x16590e0 / 0x1659400 |
| 2201128 | func | TESObjectREFR::GetActivateText (hook) | src/ActivateText.cpp:117 | activateText (phase B) | yes (6/6) | 0x513590 / 0x5135a0 / 0x5138c0 |
| 2221994 | func | HUDRollover::ShowRollover (hook) | src/ActivateText.cpp:118 | activateText (phase B) | yes (6/6) | 0xa1c880 / 0xa1c8c0 / 0xa1cbd0 |
| 2198199 | func | TESLevItem::GetOverrideName (vtable slot target) | src/LeveledListNames.cpp:101 (via VtableSlotIsSafe) | leveledNames (phase B) | yes (6/6) | 0x4511a0 / 0x4511b0 / 0x4514d0 |
| 1381610 | vtbl | TESLevItem vtable [1] | src/LeveledListNames.cpp:101 | leveledNames (phase B) | yes (6/6) | 0x248d340 / 0x248d340 / 0x24953a0 |


IDs checked: 26, missing: 0

## 3. Blocker found by the audit: load-time lookups of absent IDs

The fork declares namespace-scope globals such as `inline REL::Relocation<...> CurrentRow{ REL::ID(833923) }`. The `Relocation(ID)` constructor
resolves the ID at once, so each global calls `IDDB::id2offset` while the DLL loads. Six use IDs that exist in no 1.11.x bin
(found as `??__E...` dynamic initializers, see `phaseC/impl/trace.txt`):

| ID | Global |
|---|---|
| 526727 | Workshop (placement item) |
| 833923 | Workshop (current row) |
| 1279207 | Workshop (placement item data) |
| 775377 | ImageSpaceEffectTemporalAA::MaskS |
| 1065542 | ImageSpaceEffectPipboyScreen::bPowerArmorPipboy |
| 1378547 | CombatUtilities::fWorldGravity |

With the earlier "missing ID fails loudly" patch, the first of them would abort every 1.11.x start before any DSD code runs, for every feature.
DSD uses none of these globals.

Fix (OG slot and OG behavior untouched; `Module::IsNG()` is false there):

- New `REL::IDDB::SetStrictLookup(bool)` in `IDDB.h`, and a check in `IDDB::id2offset` (`IDDB.cpp`): on NG/AE while strict mode is off, a missing ID resolves to offset 0. Default is off.
- `F4SE_PLUGIN_LOAD` calls `REL::IDDB::SetStrictLookup(true)` right after `F4SE::Init`. Every later lookup (hooks, apply code, function-local statics) still aborts loudly on a missing ID.
- `cmake/patches/commonlibf4-runtime-detection.patch` was regenerated with `git -C extern/CommonLibF4 diff`. It applies to the pristine blobs, the reverse check passes on the patched submodule, and the forward apply fails there, so the existing CMake logic (apply, else reverse-check) keeps working.

Not run in game; verified by disassembly and build.

## 4. Absent IDs still linked into the DLL (disposition)

| Absent ID | Holder | Why it is not reached on AE |
|---|---|---|
| 84112 `__RTDynamicCast` | `FullNameCopyComponentT<false>` | OG instantiation; AE installs `<true>` (vtable test, no cast) |
| 523613, 139753 | `RuntimeText::Install` OG branch | AE branch returns before them |
| 2189084 `BSStringT::Set` | `SetViaFork`, dialogue option lambda | `SetViaFork` is the OG default of `g_stringSet`; AE replaces it before any hook exists. The lambda belongs to dialogueOptions (OFF) |
| 2193197 MemoryManager getter | `RE::malloc`, `BSTArrayHeapAllocator` | `RE::malloc` only from the OG branch of `DSD::GameMalloc`; the array allocator only from NameDiag (OFF) |
| 2688883 | `TESDataHandler::GetSingleton` | OG branch of `DSD::GetDataHandler`; NameDiag, NameRefresh (OFF) |
| 2689178, 2689189 | `TESForm::GetFormByID` | OG branch of `DSD::LookupFormByID`; NameDiag (OFF) |
| 2690301 | `GameSettingCollection::GetSingleton` | OG branch of `DSD::GetGameSettingCollection` |
| 2690919, 2285936, 827659 | DialogueOptions, NameDiag, NameRefresh | features OFF on AE |
| six globals of section 3 | `??__E` initializers | tolerated by the patch; never used |

## 5. Extra proofs

- 4797590 is the `GameSettingCollection*` global: its code references are the lazy getter (ID 2188690 `InitCollection`) in all three exes. Vtable 618060 equals the RTTI vtable of `GameSettingCollection`. At run time `DSD::GetGameSettingCollection` also compares the object's vptr with 618060 and skips the GMST group with an error log on mismatch.
- 2193092 (engine form lookup) calls lock_read 2267897 in all three exes.
- Engine `BSStringT::Set` (4471693) calls Allocate 2267872 with the memory manager singleton, the same call shape as `DSD::GameMalloc`.
- 2662549 prompt table: in `.data`, on-disk capacity/free/good are 0, sentinel at +0x18 inside `.data`. `FindPromptMapAE` checks the layout at run time.
- Hook byte guards in `src/AeHookBytes.h` (generated by `ae/gen_hookbytes.py`, checked by `ae/check_hookbytes.py`: 24 of 24 PASS for 8 hooks times 3 exes).

## 6. Fork wrappers with wrong AE IDs

| Wrapper | AE handling |
|---|---|
| `TESForm::GetFormByID`, `TESDataHandler::GetSingleton` | DSD helpers `LookupFormByID`, `GetDataHandler` |
| `MemoryManager::GetSingleton`, `RE::malloc` | DSD helper `GameMalloc` (getter 4471522, then fork `Allocate` 2267872, a valid ID). Chosen over patching the fork because the only AE-path caller is the GMST string copy; growing fork containers are used only by NameDiag (OFF) |
| `GameSettingCollection::GetSingleton` | DSD helper `GetGameSettingCollection` with vtable guard |
| `BSStringT::Set` | function pointer `g_stringSet`, set once at install (engine 4471693 on AE, fork wrapper on OG), byte-guarded |
| `fallout_cast` | not called on AE; vtable identity test |
| REGN Find 657534 | `PortedConstFields.cpp` picks 2196228 on AE |
| `GetSparseFullNameMap` | not called on AE (sparse FULL group OFF) |
| load-time globals with absent IDs | `REL::IDDB::SetStrictLookup` (section 3) |
