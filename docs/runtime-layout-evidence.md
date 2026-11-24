# Runtime layout evidence and translation safety

Scope: source-layout audit of the checked-out alandtse CommonLibF4 fork, especially `src/ConstTranslation.cpp` and `src/MiscTypes.cpp`. **Source declarations are not executable-layout verification. All in-game results remain UNVERIFIED.** No production files, tests, RE configuration, or files on D: were changed by this investigation.

## Decision

**Disable every NG/AE translation entry point until its complete dependency chain is verified.** Disabling only detours is insufficient: the GameDataReady / NewGame / PostLoadGame apply passes directly read and write game objects, and optional name diagnostics/refresh traverse many more unverified structures. A valid Address Library lookup proves an address exists, not a field offset, virtual slot, function signature, or object ABI.

The minimal safe loading policy is an early NG/AE return before hook installation and before registering the game-data listener. Keep a defensive runtime check at the listener boundary. Do not call Manager's load-order discovery, translation apply passes, or optional diagnostics/refresh first. Such a DLL may load on NG/AE but does **not** provide functioning translations there; report support as partial/disabled, not working multi-runtime translation support.

## What enabling both F4 and NG actually changes

- `extern/CommonLibF4/CommonLibF4/CMakeLists.txt:67-68` exports `ENABLE_FALLOUT_F4=1` and `ENABLE_FALLOUT_NG=1` to consumers.
- `include/REL/Common.h:10-60` changes `FALLOUT_ADDR` and `FALLOUT_REL` from `constexpr` to `inline`, and removes `constexpr` from `FALLOUT_REL_CONSTEXPR` in a multi-runtime build. `FALLOUT_CROSS_VR` is only defined when VR is also enabled, not merely because F4 and NG are enabled.
- `include/REL/ID.h:35-90` stores both IDs and selects the family at runtime. A plain `REL::ID` still holds exactly one number and does not translate an OG ID into an NG ID.
- `include/REL/Relocation.h:460-470` selects explicitly supplied OG/NG values at runtime. It cannot make ordinary member accesses runtime-aware.
- The relevant `TESForm`, `TESFullName`, `BGSLocalizedString`, `ActorValueInfo`, and `TESQuest` declarations have **no F4/NG-conditional data layout**. Their comments/static assertions describe the compiled C++ layout, not per-runtime executable proof. The fork has no separate AE layout selection for them.
- Several vtables used here are still plain OG `REL::ID` declarations: `include/RE/VTABLE_IDs.h:54` ActorValueInfo IDs 651546 / 1501706 / 530698, `:8027` TESForm 161818, `:8029` TESFullName 487988. In contrast RTTI entries have explicit pairs (`include/RE/RTTI_IDs.h:70`: ActorValueInfo 1048267 / 2764629; `:9572`: TESFullName 1098715 / 2764489). Runtime-correct RTTI alone does not validate subsequent field accesses.

Paths in this section beginning `include/` are relative to `extern/CommonLibF4/CommonLibF4/`.

## QuestObjective compile assertion: keep the OG invariant

`src/ConstTranslation.cpp:146-160` defines a local objective view; the fork only forward-declares BGSQuestObjective. Its fields are:

| Field | Source-declared offset | Source-declared storage |
|---|---:|---:|
| displayText | 0x00 | BGSLocalizedString, 8 bytes |
| ownerQuest | 0x08 | pointer, 8 bytes |
| targets | 0x10 | pointer, 8 bytes |
| numTargets | 0x18 | uint32, 4 bytes |
| index | 0x1C | uint16, 2 bytes |
| initialized | 0x1E | bool, 1 byte |
| state | 0x1F | char, 1 byte |
| flags | 0x20 | uint32, 4 bytes |

On the intended x64 ABI the tail pads to 8-byte alignment, giving **0x28**. `BSFixedString.h:251-299` declares a single `BSFixedStringCS` member and unconditionally asserts `sizeof(BGSLocalizedString) == 0x8`; neither this class nor the local objective view has an F4/NG data-layout branch. Consequently **enabling NG alone does not explain a changed QuestObjective sizeof**. Keep the existing `static_assert(sizeof(QuestObjective) == 0x28)` unchanged unless an actual compiler diagnostic demonstrates otherwise. This investigator did not compile the project. The parent subsequently reported that the dual-runtime build succeeds with this assertion unchanged; that is a parent-reported build result, not an independent build performed here.

Actionable failure handling if a build does report a size change:

1. Record the compiler's actual size and inspect packing/alignment/header selection. Do not replace the assertion with a runtime test, remove it, or assert an unproven NG size.
2. If a shared RE string type really changes ABI, give the OG objective view an explicitly OG, fixed-width 8-byte string-storage field (not an assumed new RE object representation), preserving `static_assert(sizeof(QuestObjectiveOG) == 0x28)` and adding offset assertions for index 0x1C and flags 0x20. Use that view only on OG, with a separately validated OG string adapter. This is a proposed fallback, **not an implemented or verified adapter**.
3. Keep NG/AE objective traversal disabled until both TESQuest's objective-container offset/ABI and the pointed-to objective layout/string representation are independently established.

`RE/Bethesda/BGSStoryManagerTreeForm.h:119-178` hardcodes TESQuest's TESFullName base at 0x28 and objectives at 0x118, with sizeof 0x2F0. Even a correctly sized objective view does not prove that reading `quest->objectives` is safe on another runtime.

## Direct-access inventory

Values below are **observed source declarations/claims**, not newly validated executable offsets. Unknown runtime layouts must not be enabled using these numbers.

| Item / source | Source observation | NG / AE executable evidence | Consequence |
|---|---|---|---|
| TESForm identity/type, ConstTranslation and all other form consumers | `TESForms.h:818-819,911-915`: inline GetFormID reads +0x14, GetFormType reads +0x1A, sizeof 0x20 | AE .191/.221/.240 type +0x1A confirmed below; formID and complete layout UNVERIFIED; NG UNVERIFIED | One correct type accessor does not validate all fields. |
| TESFullName component, `ConstTranslation.cpp:44-45` | `FormComponents.h:1384-1409`: fullName +8 within component, sizeof 0x10; virtual GetFullNameLength/GetFullName slots 7/8 | AE .191/.221/.240 +8 accessors confirmed below, and AVIF component base +0x20; other containing classes and NG UNVERIFIED | These offsets do not establish complete string assignment ABI or every containing form layout. |
| Sparse full-name map | `FormComponents.h:1400`: RelocationID(226372,2661402), displacement -8 | UNVERIFIED | Correct ID selection does not establish the surrounding map layout. |
| NPC name-copy hook | `ConstTranslation.cpp:121-143,467-480`: plain ID 1510422, TESFullName vtable slot 6 | UNVERIFIED | Existing slot comparison is not cross-runtime verification; avoid resolving OG-only vtable ID on NG/AE. |
| Quest objectives | local objective view and `quest->objectives` | UNVERIFIED | Both pointer-container and objective layouts required. |
| Message fields | `ConstTranslation.cpp:182-207`: buttonList/button text/shortName; `TESForms.h:2589` shortName source comment +0x70 | UNVERIFIED | Non-hook apply pass still writes objects. |
| Terminal fields | `ConstTranslation.cpp:213-312`: bodyTextItems/menuItems, nested responseText/selectionResult/flags, welcomeText/headerTextOverride | UNVERIFIED | Nested element stride/union discriminant are additional obligations. |
| Terminal pointer classification | `ConstTranslation.cpp:268-274`: dereferences first pointer and compares against .rdata | UNVERIFIED | Not a substitute for correct parent/member/union layouts. |
| GMST | `ConstTranslation.cpp:418-458`: GameSettingCollection, Setting type/string, heap-copy/SetString | UNVERIFIED | No basis here for an exception to the NG/AE apply-pass gate. |
| Ammo ONAM | `MiscTypes.cpp:36-45`: ammo->shortDesc, source comment +0x178 | UNVERIFIED | Full containing-class layout needed. |
| Alchemy DNAM | `MiscTypes.cpp:47-56`: alchemy->data.addictionName, source comment +0x1A8+0x20 | UNVERIFIED | Nested structure layout needed. |
| Door ONAM/CNAM | `MiscTypes.cpp:58-78`: altOpenText/altCloseText, source comments +0x108/+0x110 | UNVERIFIED | Full containing-class layout needed. |
| AVIF ANAM | `MiscTypes.cpp:80-94`: raw OG +0x160; comment attributes AE +0x1A0 to unavailable FallHook | AE .191/.221/.240 +0x1A0 confirmed by loader dispatch below; OG, NG and earlier AE UNVERIFIED independently | Proven offset alone does not authorize enabling this translation path. |
| Additional const fields | `PortedConstFields.cpp`: NPC shortName, load-screen loadingText, magicItemDescription, reference extraList/mapMarkerData, region dataList, perkEntries plus raw payload offsets | UNVERIFIED | Gate with ConstTranslation, not merely its hook. |
| Template names / INNR | `TemplateNames.cpp`: TESBoundObject objectTemplate.items, per-item fullName, naming ruleSets; TESDataHandler form arrays | UNVERIFIED | Called by ConstTranslation's apply pass and own-name lookup. |
| Translation load order | `Manager.cpp:11-22`: TESDataHandler singleton/files, TESFile compile indices/flags/filename | UNVERIFIED | Do not call EnsureLoaded/Reload before the safe runtime gate. |
| Runtime descriptions / quest journals | `RuntimeText.cpp`: TESDescription RTTI, TESQuest GetFormID/GetFile, BSStringT::Set output | UNVERIFIED | Both detours and table-building accesses require gating. |
| Body parts / chargen | `BodyPartsChargen.cpp`: TESRace faceRelatedData, tintingTemplate groups/entries, morph groups/presets, facialBoneRegions; BGSBodyPartData partArray/partName | UNVERIFIED | File-derived counts are not proof of in-memory offsets. |
| Name diagnostics / refresh | `NameDiag.cpp`, `NameRefresh.cpp`: cells/references/locks, player inventory, stack/extras, displayName flags/ownerQuest, instance data, world cellMap/persistentCell, object templates | UNVERIFIED | Optional diagnostics can crash while only reading. Disable them too. |

## AVIF ANAM: what the header does and does not prove

`RE/Bethesda/ActorValueInfo.h:204-235` declares TESForm at 0, TESFullName at 0x20, TESDescription at 0x30, `modifiedCallback` (`std::function`) at 0x48, editorID at 0x88, dependentActorValues at 0x98, `derivationFunction` (`std::function`) at 0x110, enumNames at 0x150, abbreviation at **0x1A0**, and sizeof **0x1D0**, unconditionally.

This is not a runtime accessor. In particular, two embedded `std::function` objects make historical compiler/library ABI changes a relevant concern; this investigation has not established the cause of the alleged 0x40 difference. The file does not label the layout as OG, NG, or AE and cannot prove all three share it. The `MiscTypes.cpp` comment conflates 1.11 AE with NG: **1.10.98x and 1.11.x must be treated separately**. The unavailable FallHook references are a provenance claim, not evidence produced here. The raw existing OG +0x160 is also not independently confirmed by this source audit.

Required binary proof: identify ActorValueInfo by RTTI/vtable in each executable, follow its Load handler to the ANAM subrecord case, and establish the localized-string destination relative to the original object base. A bare `+0x1A0` instruction anywhere in .text is insufficient. Also establish localized-string assignment ABI. For NG no executable is supplied, so Address Library membership alone cannot prove its ANAM offset. For AE, confirmation on 1.11.191 does not automatically prove .137/.159/.169/.221/.240.

## Initial disassembly attempt and reproducibility limits

Installed Python modules were checked with `importlib.util.find_spec`: both **capstone** and **pefile** are already available in Python 3.11 site-packages. No installation was attempted. Address Library files were opened read-only to inspect their headers; their first uint64 count and fixed-size pair format match the repository decoder. The repository's AddressLibDecoder writes a sibling `.txt` beside its input, so **do not run it directly against the D: bins**.

The disassembly was run as inline read-only Python without a persisted script. The successful follow-up evidence is recorded below. The RTTI-based method was:

1. Find `.?AVActorValueInfo@@` (likewise TESFullName/TESForm); MSVC type descriptor begins 16 bytes before the name.
2. Find image-relative CompleteObjectLocator structures with signature 1, matching type-descriptor RVA at +12, and matching self-RVA at +20; retain the subobject offset.
3. Find the absolute image-base-plus-COL pointer preceding each vtable. Follow ActorValueInfo's primary vtable Load slot 9 to its record-subtype dispatch, and TESFullName slots 7/8 or TESForm slot 0x19 for accessors.
4. Record executable/version, RVA, instruction bytes, base-register derivation, and destination offset. Check actual bytes before trusting a slot number. Do not assert that on-disk OG bytes are unpacked.

The successful inline follow-up below supersedes the initial lack of disassembly for the bounded facts it establishes. All other entries retain UNVERIFIED status; this document makes no independent build, test, or in-game-success claim.

## Successful inline read-only executable investigation

Inline Python read the executables directly with pefile/capstone; no analysis script was persisted. PE VS_FIXEDFILEINFO independently reported versions **1.11.191.0**, **1.11.221.0**, and **1.11.240.0**, respectively. All addresses below are **RVAs**, not process addresses.

Executable abbreviations:

- **191**: `Fallout4.exe`
- **221**: `Fallout4.exe`
- **240**: `Fallout4.exe`
- **OG**: `Fallout4.exe`

### RTTI establishes which object the loader receives

The inline scan used the exact MSVC type name, valid image-relative CompleteObjectLocator signature/self-RVA, then absolute COL-pointer references preceding the vtables. It did not depend on an assumed AE vtable Address Library ID.

| Runtime | ActorValueInfo type descriptor | Primary COL / vtable | TESFullName subobject COL / vtable | TESDescription subobject COL / vtable |
|---|---:|---|---|---|
| 191 | 0x2F94618 | 0x295E438 / 0x25005D8 | offset 0x20: 0x295E4E0 / 0x2500830 | offset 0x30: 0x295E508 / 0x2500880 |
| 221 | 0x2F94658 | 0x295E438 / 0x25005D8 | offset 0x20: 0x295E4E0 / 0x2500830 | offset 0x30: 0x295E508 / 0x2500880 |
| 240 | 0x2F9F6A8 | 0x2967BF8 / 0x2508648 | offset 0x20: 0x2967CA0 / 0x25088A0 | offset 0x30: 0x2967CC8 / 0x25088F0 |

### AVIF ANAM is at +0x1A0 in all three supplied AE executables

The primary vtable slot 9 resolves to a record loader. At entry it saves the original object base: `4c8bf9` = `mov r15,rcx` at Load+0xB. The function verifies record type byte 0x62 (AVIF), obtains successive subrecord types, and dispatches on them. There is exactly one `cmp eax,0x4D414E41` (little-endian **ANAM**) in the inspected 0x420-byte Load window for each executable. On equality it passes **original object base +0x1A0** as the destination to the same localized-string reader used for FULL:

| Runtime | Load RVA | ANAM compare RVA / bytes | Destination RVA / bytes | Call RVA / target |
|---|---:|---|---|---|
| 191 | 0x6FD570 | 0x6FD810: `3d414e414d` | 0x6FD81B: `498d8fa0010000` | 0x6FD825: `e8663cc5ff` -> 0x351490 |
| 221 | 0x6FD5A0 | 0x6FD840: `3d414e414d` | 0x6FD84B: `498d8fa0010000` | 0x6FD855: `e8463cc5ff` -> 0x3514A0 |
| 240 | 0x6FD8B0 | 0x6FDB50: `3d414e414d` | 0x6FDB5B: `498d8fa0010000` | 0x6FDB65: `e8563cc5ff` -> 0x3517C0 |

The compare is followed by `0f85a1000000` (`jne` to the next-subrecord loop); the equality path executes `lea rcx,[r15+0x1A0]`, `488bd5` (`mov rdx,rbp`, the file argument), and the call. The FULL case in that same loader passes the subobject at +0x20 (`498d4f20`) to the TESFullName loader:

| Runtime | FULL subobject LEA RVA | TESFullName loader RVA | FULL check / member LEA / localized-reader call RVAs |
|---|---:|---:|---|
| 191 | 0x6FD832 | 0x315C80 | 0x315CA2 / 0x315CA9 / 0x315CB0 |
| 221 | 0x6FD862 | 0x315C90 | 0x315CB2 / 0x315CB9 / 0x315CC0 |
| 240 | 0x6FDB72 | 0x315FB0 | 0x315FD2 / 0x315FD9 / 0x315FE0 |

For each FULL helper the instructions are `3d46554c4c` (`cmp eax,0x4C4C5546`, FULL), `750c` (skip if not FULL), `488d4f08` (`lea rcx,[rdi+8]`, where rdi preserves its incoming component pointer), `488bd3`, and `e8dbb70300`, resolving to the exact same per-version reader target as ANAM. This ties the AVIF ANAM destination to a localized text field rather than an arbitrary occurrence of displacement 0x1A0.

**Bounded conclusion:** AE .191/.221/.240 ANAM destination +0x1A0 is now independently established, as are the AVIF TESFullName base +0x20 and FULL field +8 within that component. This does not prove NG or earlier AE, nor every string-pool allocation/assignment operation used by the plugin. Keep the NG/AE feature gate in place.

### TESFullName accessors and TESForm type byte

Standalone TESFullName RTTI found vtable 0x24625E0 (191/221) and 0x246A640 (240). ActorValueInfo's +0x20 component vtables point to the same accessor functions.

| Runtime | GetFullNameLength slot 7 RVA / tail-jump bytes | GetFullName slot 8 RVA / tail-jump bytes | TESForm type slot 0x19 RVA |
|---|---|---|---:|
| 191 | 0x255670: `4883c108 e917bc0f00` -> 0x351290 | 0x255660: `4883c108 e987ba0f00` -> 0x3510F0 | 0x2622B0 |
| 221 | 0x255670: `4883c108 e927bc0f00` -> 0x3512A0 | 0x255660: `4883c108 e997ba0f00` -> 0x351100 | 0x2622B0 |
| 240 | 0x255990: `4883c108 e927bc0f00` -> 0x3515C0 | 0x255980: `4883c108 e997ba0f00` -> 0x351420 | 0x2625D0 |

All accessors first execute **`add rcx,8`**, corroborating the component-relative string location. The 191 GetFullName target calls a string accessor at 0x167CA00, checks the returned prefix, and conditionally returns pointer+13 (`488d430d` at 0x35111B), consistent with localized-string prefix handling. This is supporting evidence, not complete proof of the plugin's assignment ABI.

For the type accessor each executable contains **`0fb6411a c3`** = `movzx eax,byte ptr [rcx+0x1A]; ret`. Standalone TESForm vtables are 0x246F110 (191/221) and 0x2477170 (240); AVIF primary slot 0x19 uses the same target. This proves the type-byte offset on these three builds, **not** formID+0x14 or total TESForm size.

### OG and remaining unknowns

OG RTTI is readable: ActorValueInfo TD 0x36CFBD0, primary COL 0x30DC470, primary vtable 0x2CEC9E8, Load slot 9 -> 0x69F810. However bytes there are `7828fe361fa3cf6101f13a77997ca72a1aa4072a4ff4b878ef119c78e5b526d4`, not a plausible unpacked record-loader prologue. TESFullName TD 0x36CE220 / COL 0x30A74D0 / vtable 0x2C4C1B8 slot 8 -> 0x52980 likewise yields `464c66ee0bd18d80503bb6d3e22cbdb7e62dd76d13a282df00d6e5a3ee6f5d3b` (including nonsensical privileged I/O in naive decoding). Do not infer OG field offsets from those encrypted/packed-looking instruction bytes. OG ANAM +0x160 remains a pre-existing source claim rather than independent binary confirmation here.

No NG executable exists among the supplied inputs. No executable was supplied for AE .137/.159/.169. Those layouts, QuestObjective/TESQuest containers, all other table rows, and complete localized-string write compatibility remain UNVERIFIED. Parent reports the dual-runtime build passes the existing objective assertion unchanged; successful compilation does not resolve these runtime unknowns. Every in-game result remains UNVERIFIED.
