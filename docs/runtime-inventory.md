# Runtime-sensitive dependency inventory

## Scope and evidence status

This is a **source inventory, not a runtime-support certification**. All tracked files in `src/` and `include/` were read completely, including the game-independent core and public headers. Relevant CommonLibF4 definitions were inspected to distinguish direct memory access, compile-time base adjustment, engine calls, and runtime-selected relocations. Source references below describe the current checkout; no production code was changed.

Only this document is an output. No executable or Address Library bin was disassembled/decoded for this inventory, no files on `D:` were written, and no build, tests, commits, or branch changes were performed. **Every NG/AE code address, signature, slot, and layout remains UNVERIFIED against executable evidence in this investigation.** Existing source comments are attribution, not new verification. OG constants below are existing source values, also not new binary proofs.

Target families: OG 1.10.163; NG 1.10.980 / 1.10.984; AE 1.11.137 / .159 / .169 / .191 / .221 / .240. Do not extrapolate a layout from NG to AE or between AE revisions. An Address Library hit proves a mapping exists, not that its target has the intended signature or object layout.

Notation:

- `CL/` = `extern/CommonLibF4/CommonLibF4/`.
- `B/` = `CL/include/RE/Bethesda/`; `G/` = `CL/include/RE/Scaleform/GFx/`.
- Offsets and sizes are hexadecimal unless explicitly decimal. Offsets are relative to the named object/subobject, not necessarily the complete form.
- `OG / NG` ID pairs below are **header-declared** `RelocationID` values, not bin lookups. There is no separately proven AE value.
- `R`, `W`, `C`, `H` mean read, write, engine call, and hook respectively.

## 1. Admission, initialization, and global prerequisites

| Dependency | Source | Current behavior / sensitivity |
|---|---|---|
| Build-time runtime selection | `CMakeLists.txt:11-16` | F4 ON, NG OFF, VR OFF. This inventory does not enable another runtime. A single-runtime library build can compile away runtime choices. |
| Plugin version export | `src/main.cpp:126-135` | `UsesAddressLibrary(true)`, `IsLayoutDependent(true)`, fixed compatible list containing only `RUNTIME_1_10_163`. Advertising address-library use does not remove layout dependence. |
| OG Query export | `src/main.cpp:138-153` | Rejects editor and every runtime other than 1.10.163; keep this boundary distinct from modern Version metadata. |
| F4SE bootstrap | `src/main.cpp:155-192` | `Init`, settings/logging, 1024-byte trampoline (`1 << 10`, line 175), five install entry points, messaging registration. All install work precedes listener registration. |
| Runtime classification | `CL/src/REL/Module.cpp:45-56` | minor version 2 -> VR; otherwise build component >163 -> NG, else F4. This misclassifies AE .137/.159 as F4. A corrected classifier is a prerequisite, not proof of member compatibility. |
| Runtime member helper semantics | `CL/include/REL/Relocation.h:662-684` | Two-offset `RelocateMember` chooses **F4-and-NG versus VR**, not OG versus NG. NG ON alone does not fix desktop member offsets. |
| Lifecycle / timing | `src/main.cpp:44-122` | GameDataReady(false) clears runtime-description tables, captured options, and cached file layouts before store reload. Ready(true) runs every apply module. NewGame/PostLoadGame reapply const, misc, body/chargen then optionally refresh instance names. Diagnostics also run at PreSaveGame. |
| Translation-store reload precondition | `include/Manager.h:25-27`; `src/Manager.cpp:46-78` | Store pointers used by hooks must not outlive/reenter a reload. Reset/enable lifecycle must be preserved if runtime gating is introduced. |
| Shared SDK ABI | `include/PCH.h:3-4`; `src/main.cpp:44-122,155-192` | F4SE interface versions, Message layout/type/data conventions and callback calling convention (`F4SEAPI` for Scaleform) are external ABI dependencies. No raw F4SE-interface offsets are hardcoded locally. |

In the inspected snapshot there is **no per-runtime capability gate inside the feature modules**. Settings gates and null/type/count checks are not runtime proof. **Later update:** a `Runtime.h` load-admission helper was added that gates all NG/AE translations before trampoline allocation, hooks, and listener registration. Final spot-check confirms `src/Runtime.h:7-33` explicitly classifies the listed versions and returns true for `CanTranslate` only on OG (legacy behavior, not a new binary certification); `src/main.cpp:180-185` returns before trampoline allocation and installs on other versions, with an additional listener guard at `44-48`. The parent-reported build was not reproduced here. The recommendations below describe the inspected feature bodies and future granular enablement, not a claim that the parent's load gate is absent.

## 2. Complete directly named hook / relocation inventory

### 2.1 Installed hooks (nine prologues and one vtable replacement)

All prologue sites pass a **single OG ID** to `HookUtil::InstallPrologueHook`; none supplies an NG/AE ID pair. Thunk signatures are part of the contract even if the address resolves.

| Feature / target | Existing ID / slot | Source (signature; installation) | ABI and dependencies |
|---|---|---|---|
| Spoken INFO NAM1: DialogueResponse constructor | 755245 | `src/Dialogue.cpp:52-71;114-117` | Returns `DialogueResponse*`; args `(this, TESTopic*, TESTopicInfo*, TESObjectREFR* speaker, TESResponse*, TESQuest*)`. Writes returned/fallback-this text at +00 after original. |
| NPC template FULL copying | 1510422; TESFullName vtable `[0]`, slot 6 | `src/ConstTranslation.cpp:120-140;425-437` | `void(TESFullName*, BaseFormComponent*)`. RTTI-casts source to TESForm, checks NPC type and writes source name before original. Compares vtable slot with ID target before patching. |
| Description reader | 523613 | `src/RuntimeText.cpp:104-121;156-160` | `void(TESDescription*, BSStringT<char>&, const TESForm*)`; original first, then replace output by component identity. |
| Quest journal reader | 139753 | `src/RuntimeText.cpp:123-151;156-160` | `void(TESQuest*, BSStringT<char>&, uint16 stage, uint8 item)`; original first. |
| Captured player dialogue INFO | 781358 | `src/DialogueOptions.cpp:177-192;588` | Returns `TESTopicInfo*`; args `(BGSSceneActionPlayerDialogue*, BGSScene*, TESObjectREFR*, uint32 responseType)`. Original first, then chain capture. |
| Vanilla wheel text | 643840 | `src/DialogueOptions.cpp:194-249;589` | `void(uint8 button, BSStringT<char>&, uint8 challengeLevel, bool enabled)`; replace input text before original. |
| Late XDI registration monitor | 1360149 | `src/DialogueOptions.cpp:484-524;594` | `bool(ObjectInterface*, void* data, const char* name, const Value&, bool displayObject)`; calls original and possibly wraps registration. CommonLib header has OG/NG pair 1360149/2286589, but local install uses OG only. |
| Activation context | 1077221 | `src/ActivateText.cpp:50-65;112` | `bool(TESObjectREFR*, BSStringT<char>&)`; original first; remembers base ID and tick. |
| HUD primary action replacement | 414044 | `src/ActivateText.cpp:67-101;113` | `void(void* rollover, void* parameters)`; temporarily overwrites parameters+08 entry pointer around original. |
| Leveled item override-name reader | TESLevItem vtable `[1]`, slot 0A | `src/LeveledListNames.cpp:16-17,31-49,89-97` | `const char*(TESLeveledList*)`; original first; subtracts 68 from this for TESForm ID. Stores original before writing slot; installs once only if translated forms exist. |

### 2.2 Other directly named relocations and explicit vtable symbols

| Item | Existing source value | Source / use |
|---|---|---|
| XDI array element accessor | `REL::ID(827659)` | `src/DialogueOptions.cpp:265-275`: `bool(ObjectInterface*, void*, uint32, Value*)`; lazy static constructed only after raw pointers pass null checks. Not supplied by the fork's Value API. |
| REGN data-list Find | `REL::ID(657534)` | `src/PortedConstFields.cpp:95-106,119-120`: `void*(TESRegionDataList*, uint32)`; requested kind is 4. |
| Name-refresh diagnostic address read | `REL::ID(457340)` | `src/NameRefresh.cpp:297-308`: reads/logs first **16 bytes**, not a hook. Engine repair itself uses library `RelocationID(457340,2190179)`. Disabling detail logging avoids this OG-only lookup, not the repair. |
| TESFullName vtable | `RE::VTABLE::TESFullName[0]` -> ID 487988 | `src/ConstTranslation.cpp:429-432`; `CL/include/RE/VTABLE_IDs.h:8029`. Reads slot 6 for consistency check. |
| TESLevItem secondary vtable | `RE::VTABLE::TESLevItem[1]` -> ID 1381610 | `src/LeveledListNames.cpp:93-95`; `CL/include/RE/VTABLE_IDs.h:8042` declares primary 718716 and secondary 1381610. Only secondary is used. |
| Perk-entry exact vptr identity | `RE::VTABLE::BGSEntryPointPerkEntry[0]` -> ID 846981 | `src/PortedConstFields.cpp:152,158`; `CL/include/RE/VTABLE_IDs.h:1912`. R, not a hook. |
| Activate-choice exact vptr identity | `RE::VTABLE::BGSEntryPointFunctionDataActivateChoice[0]` -> ID 859152 | `src/PortedConstFields.cpp:153,162`; `CL/include/RE/VTABLE_IDs.h:1905`. R, not a hook. |

**Comment-only values, not active relocations/hooks:** `875508` (SubtitleManager fallback deliberately absent, `src/Dialogue.cpp:74-75`); `2196228` and purported RVA `0x3EDD70` (`src/PortedConstFields.cpp:98-101`); purported loader RVAs `0x623730`, `0x61A000`, slots 7 and 5 (`src/PortedConstFields.cpp:131-135`). The latter comments call 1.10.980 “AE”; under this brief it is NG. They cannot prove AE. FallHook attribution is present but its source is not available here. The “encrypted OG exe” comments likewise are not evidence produced by this inventory.

## 3. Raw offsets, patterns, mirrored layouts and patch sizes

| Item | Existing assumption | Source / failure mode |
|---|---|---|
| HUD parameter primary action slot | +08 holds BSFixedString entry pointer | `src/ActivateText.cpp:30-32,84-90,97-98`. Raw pointer overwrite then restore. Pointer >=0x10000 check (line 86) excludes low addresses only; it does not prove string type, writability, or offset. |
| Fixed-string representation | first pointer of `BSFixedStringCS` is pooled Entry* | `src/ActivateText.cpp:121,145-146`. Leaked vector retains entries; copying entry pointer is not copying char*. |
| QuestObjective mirror | size 28; displayText 00, ownerQuest 08, targets 10, numTargets u32 18, index u16 1C, initialized bool 1E, state char 1F, flags u32 20 | `src/ConstTranslation.cpp:143-156`. Only displayText and index used (`166-170`). Compile-time size assertion proves local compiler layout only, not engine layout. |
| DialogueResponse prefix mirror | BSFixedStringCS text +00; comment says whole engine object 40 | `src/Dialogue.cpp:12-17,65-66`. Local type contains only prefix, no 40-byte assertion/allocation/stride. |
| Prompt-map discovery | bytes `48 83 C6 18 48 83 C7 18`; search .text; instruction at match+0C; signed disp32 at instruction+3; instruction length 7; table = operand-28 | `src/Dialogue.cpp:77-101`. Only **first match** used. Checks end bound, not unique hit count, opcode at +0C, operand/table section, or hash-map validity. |
| Terminal union discrimination | flags exactly signed byte 8; pointed-to first word inside executable .rdata implies form rather than string | `src/ConstTranslation.cpp:257-272`. Dereferences pointer before structural proof. Section-range heuristic is not runtime layout validation. |
| Leveled-list this adjustment | base subobject +68, vtable index 1, slot 0A | `src/LeveledListNames.cpp:9-17,40,93-95`. Comment attributes .191 disassembly and OG in-game behavior; no new proof. |
| AVIF abbreviation | **writes +160 unconditionally** | `src/MiscTypes.cpp:76-87`. Comments contrast +1A0 in .191 / library header. Must not use +160 on another runtime merely because As succeeds. NG +1A0 is not proven by the comment's “AE/NG” wording. |
| REGN map-data name | returned map data +10 localized string; GetID virtual slot 3; kind 4 | `src/PortedConstFields.cpp:95-125`. No local map-data type or size assertion. |
| PERK entry/function data | entry+18 -> function data; data+18 -> label; data+28 -> u16 EPF3 index; comment says data object size 30, second EPF3 u16 at +10 | `src/PortedConstFields.cpp:129-177`. Exact vptrs filter subclasses; index equality does not establish safe offsets and a bogus pointer is dereferenced before equality check. |
| GFx Value mirror | ObjectInterface* 00, int32 type 08, pad 0C, void* data 10, size_t dataAux 18; local size equals Value size (20 in fork) | `src/DialogueOptions.cpp:253-274`; `G/GFx_Player.h:1017-1023`. Size equality alone does not prove member positions. |
| Extra custom-name marker | BSExtraData flags+10 bit 0 | `src/NameRefresh.cpp:120-125`. Safety depends on flag/member semantics as well as offsets. |
| String-prefix rule | localized prefix length decimal 13 (`<ID=...>`) | `include/TextWrite.h:8-24`; `B/BSFixedString.h:259-274,298-304`. Semantic representation dependency, not engine object offset. |
| Prologue patch size | decode whole instructions until >=5 bytes, bounded by 16-byte window | `src/HookUtil.cpp:7-9,151-163`. No predetermined per-hook copy length or target-byte signature. |
| Original trampoline | copied prologue +14-byte absolute indirect jump (`FF 25 00 00 00 00` then uint64 return address) | `src/HookUtil.cpp:175-193`; x64 pointer/ABI dependency. |
| Target patch | 5-byte branch then 90 NOP padding to decoded length | `src/HookUtil.cpp:194-198`. Allocation within relative reach delegated to F4SE trampoline. |
| Generic call-site helper | template default SIZE=5 | `include/HookUtil.h:5-12`. Defined but **unused** by current sources; no hidden call-site offsets. |

Hook decoder contract (`src/HookUtil.cpp:17-148`): handles a limited x64 opcode set; REX 40–4F and legacy prefixes 66/67/F2/F3/2E/36/3E/26/64/65; ModRM/SIB/disp8/disp32; selected immediate widths 1/4/8. Refuses recognized E8/E9/EB, short/near conditional branches and RIP-relative operands rather than relocating them. This is not a full disassembler and not an identity check. Supported bytes at a wrong ID still get patched. A preexisting relative detour is generally rejected, but arbitrary mod-patched prologues are not exhaustively recognized. Also, prologue patch publication precedes assigning `T::func` (`include/HookUtil.h:25-29`), unlike the explicit original-first ordering of the vtable hook; installation assumes no concurrent invocation in that interval.

## 4. RE memory dependencies grouped by feature

The following are **header/source-declared layouts**. Unless a runtime selector is specifically mentioned, the accessed member is an ordinary C++ field with a compile-time displacement: no NG/AE correction occurs automatically. All rows inherit form/type, container/string, allocation and locking prerequisites in section 5.

### 4.1 Manager / load-order identity

`src/Manager.cpp:6-25` reads `TESDataHandler::files` and TESFile accessors. `B/TESDataHandler.h:377-384` declares formArrays+68, interiorCells+F58, files+FB0; non-VR compiled file collection+FC0 (`385-386`). `GetSingleton` uses 711558/2688883 (`97`). `B/TESFile.h:98-102,185-210` declares filename char[260]+70, flags u32+334, compileIndex u8+370, smallFileCompileIndex u16+372. IsLight tests flag bit 9; inactive index is FF. These are prerequisite reads for **every** translation feature, including otherwise game-independent JSON loading.

Body/quest file readers (`src/BodyPartsChargen.cpp:42-109`; `src/RuntimeText.cpp:60-95`) additionally depend on `TESForm::GetFile(0)`, TESFile filename and the “defining plugin, not overriding plugin” interpretation.

### 4.2 ConstTranslation: base FULL and NPC name-copy hook

- `src/ConstTranslation.cpp:38-56`: `As<TESFullName>()`, W `TESFullName::fullName` at component+08 (component size 10, `B/FormComponents.h:1477-1508`); otherwise R/W sparse map value. Sparse-map relocation includes **-08** adjustment (`1496-1500`).
- `src/ConstTranslation.cpp:120-137`: `fallout_cast<TESForm*>(BaseFormComponent*)`, R form type and player ID, template-aware own-name resolution and W full name before copy. TESNPC RTTI is explicitly avoided; its fork class erroneously aliases TESActorBase RTTI/VTABLE (`B/TESBoundAnimObjects.h:431-433`). This hook still depends on BaseFormComponent and TESForm RTTI IDs and dynamic-cast engine function.
- `src/ConstTranslation.cpp:110-115` calls `TemplateNames::OwnNameIndex`, so the NPC copy hook also traverses object-template layouts; proving TESFullName alone is insufficient.
- No `AddChange` call is made for direct translations. Do not replace these writes with a save-marking API during a port.

### 4.3 ConstTranslation: objectives, messages and terminals

| Feature / source | Access chain (R unless W marked) | Declared layout evidence |
|---|---|---|
| QUST NNAM, `src/ConstTranslation.cpp:159-172` | TESQuest.objectives -> pointer -> local QuestObjective.index; W displayText | `B/BGSStoryManagerTreeForm.h:160`: objectives+118; local mirror in section 3. Quest size 2F0. |
| MESG ITXT, `175-190` | BGSMessage.buttonList -> MESSAGEBOX_BUTTON*; W button.text | `B/TESForms.h:2551-2558,2587`: list+58; text+00; button stride/size 10. Null buttons skipped when counting. |
| MESG NNAM, `193-200` | W BGSMessage.shortName | `B/TESForms.h:2589`: +70; message size 80. |
| TERM BTXT, `205-215` | bodyTextItems.size/index; W BodyTextItem.itemText | `B/TESBoundAnimObjects.h:590-597,643`: array+1A0; itemText+00, element size10. |
| TERM ITXT/RNAM/UNAM, `218-272` | menuItems.size/iteration; W itemText/responseText or *selectionResult.displayText; R flags | same header `600-619,644`: array+1B8, itemText00, responseText08, union10 (size08), flags22, element size28. Response/result counts skip entries without qualifying fields. |
| TERM WNAM/NAM0, `275-291` | W welcomeText/headerTextOverride | same header `645-648`: header+1D0, welcome+1D8; terminal size1E0. |

### 4.4 ConstTranslation: GMST DATA

`src/ConstTranslation.cpp:379-420`: GameSettingCollection singleton -> GetSetting(editor_id) -> type/string read -> W string pointer via SetString. Copies text with `RE::malloc`, not CRT allocation. `B/Settings.h:257-262,324-331,375-395` supplies Setting value union+08 and key pointer+10 (size18), GameSettingCollection.settings tree+118 (collection size138). Type comes from key first character `s`/`S`, not a stored enum. Matching a setting by name still traverses a layout-sensitive engine tree and interns a fixed string.

### 4.5 Ported const fields

| Type / source | Accessed members / layout | Notes |
|---|---|---|
| LSCR DESC, `src/PortedConstFields.cpp:22-32` | W TESLoadScreen.loadingText+30 | `B/TESForms.h:1856`; size38. |
| MGEF DNAM, `35-45` | W EffectSetting.magicItemDescription+198 | `B/TESForms.h:1202`; size1B0. |
| NPC_ SHRT, `48-61` | W TESNPC.shortName+298 | `B/TESBoundAnimObjects.h:548`; size308. Player skip uses form IDs. Helper `ApplyNpcShortName` (`64-71`, declared in `include/PortedConstFields.h:28-30`) has no caller in current src; normal indexed writer is live. |
| REFR FULL, `74-92` | TESObjectREFR.extraList+100 -> GetByType<ExtraMapMarker> -> mapMarkerData+18 -> TESFullName base+00 -> W fullName+08 | `B/TESObjectREFRs.h:943`; `B/BSExtraData.h:381-401`; MapMarkerData size18, extra size20. Requires extra-type enumeration and extra-list layout. |
| REGN RDMP, `95-125` | TESRegion.dataList+20 -> engine Find(kind4) -> W returned object+10 | `B/TESForms.h:1402`; raw function/map details section3. |
| PERK EPF2, `129-181` | BGSPerk.perkEntries array+68 -> exact vptr -> entry+18 pointer -> exact choice vptr -> u16+28; W localized label+18 | `B/TESForms.h:2120`; all raw offsets section3. Two static vtable relocations resolve on first writer invocation even if perkEntries empty. |

### 4.6 TemplateNames and INNR

`src/TemplateNames.cpp:38-53,59-93`:

- Reads form type to admit only ARMO / WEAP / NPC_. Static cast from TESForm to TESBoundObject assumes common primary base layout; `objectTemplate` component+30 (`B/TESBoundObjects.h:111`).
- `BGSMod::Template::Items::items` at component+08, size20 (`B/BGSMod.h:240-267`), hence the nested array is at complete bound object+38 under this declaration.
- Each `Template::Item` has TESFullName base00; fullName is item+08; item size38 (`B/BGSMod.h:217-237`). Reads nonempty names to choose indexed slot; writes own or template localized string.
- Own TESFullName compile-time base offsets differ by concrete type: ARMO/WEAP+68 (`B/TESBoundObjects.h:153,528`); NPC through TESActorBase+120 (`B/TESBoundAnimObjects.h:371`). Name field then +08. `As` does not dynamically repair those offsets.

`src/TemplateNames.cpp:95-148`: TESDataHandler.GetFormArray<BGSInstanceNamingRules> -> each ruleSets[10]+28 -> RuleSet (BSTArray, size18) -> RuleData (size38). R/W rule.text at rule+00. `B/TESForms.h:3273-3299`. Matching original/current text and rule ordinal guards translation identity, **not array/object layout**. Diagnostics additionally read ARMO.instanceNamingRules through BGSInstanceNamingRulesForm base+240 / member+08 (`src/NameDiag.cpp:307-317`; `B/TESBoundObjects.h:164`; `B/FormComponents.h:852-861`).

### 4.7 MiscTypes

| Type / source | Accessed member | Header declaration |
|---|---|---|
| AMMO ONAM, `src/MiscTypes.cpp:35-44` | W TESAmmo.shortDesc+178 | `B/TESBoundObjects.h:767`; ammo size1B0. |
| ALCH DNAM, `46-54` | W AlchemyItem.data.addictionName: +1A8 +20 = +1C8 | `B/MagicItems.h:250-257`; Data size28, AlchemyItem size1E0. |
| DOOR ONAM/CNAM, `56-74` | W TESObjectDOOR.altOpenText+108 / altCloseText+110 | `B/TESBoundAnimObjects.h:185-186`; door size150. |
| AVIF ANAM, `76-88` | W raw localized string+160 | Header instead declares abbreviation+1A0, `B/ActorValueInfo.h:225-240`, class size1D0. Actual selected writer ignores header member. |
| INGR YNAM, `90-92,143-145` | **No memory access/write**; counts skipped entries | +158 sound and four-byte value are comment-only claims. Do not create a text writer for it. |

### 4.8 BodyPartsChargen

`src/BodyPartsChargen.cpp:194-225`: BGSBodyPartData.partArray[26]+58 -> BGSBodyPart* -> W partName+40. `B/TESForms.h:2173,2226`; both body-part and body-part-data declared size150. Array index comes from disk BPND byte65, **not runtime PART_DATA.type+95** (`include/core/ChargenLayout.h:19-22`; `src/core/ChargenLayout.cpp:11-19`). Runtime PART_DATA is not dereferenced by this feature.

`src/BodyPartsChargen.cpp:118-176,229-296`: TESRace.faceRelatedData[2]+698 (male then female pointers), race size6D8; FaceRelatedData size68 (`B/TESRace.h:114-132,175-180`). FaceRelatedData offsets below are **derived from the header's consecutive pointer declarations**, not annotated runtime proofs:

| Type | Traversal and W destination | Nested declared layouts |
|---|---|---|
| RACE TTGP | face.tintingTemplate (+00) -> Groups.groups+00 -> Group.name+00, Group.entries+10 -> Entry.name+08 | `B/BGSCharacterTint.h:87-93,118-136`: Entry size20; Group size28; Groups size18. |
| RACE MPPN | face.morphGroups (+38) -> array of Group* -> Group.presets+08 -> Preset.name+00 | `B/BGSCharacterMorph.h:25-46`: preset stride20, group size40. |
| RACE FMRN | face.facialBoneRegions (+40) -> array of FacialBoneRegion* -> region.name+28 | `B/BGSCharacterMorph.h:66-80`: region size90. |

The count-vs-file guard happens **after** `a_build(race)` has traversed these pointers (`src/BodyPartsChargen.cpp:258-261`). It prevents a semantically wrong write when counts differ; it cannot prevent invalid reads from an unsupported layout. Gate before building slots, preferably before Apply/ReadLayouts if shared form/file layouts are unproven.

### 4.9 Dialogue responses and prompt map

`src/Dialogue.cpp:21-49`: TESTopicInfo.dataInfo+30; responses wrapper+48; wrapper.head+00; TESResponse.pNext+08; TESResponse.data+10 and its responseID byte+02 (therefore response+12); responseText localized string+00 used by trace logging. `B/TESForms.h:1711-1740,1799-1809`; TESResponse size18, TESTopicInfo size50. The response offsets are derivations from declared field order where the header omits annotations. Maximum owner-chain depth decimal16 is a traversal guard, not a member offset.

`src/Dialogue.cpp:119-142`: traverses discovered `BSTHashMap<TESTopicInfo*,BSFixedString>`, R key form ID, W prompt pooled string. This map is entirely independent of constructor hook installation. Disabling `Dialogue::Install` does **not** disable unsafe `ApplyPrompts` scanning/iteration.

### 4.10 DialogueOptions: vanilla context and XDI

- `src/DialogueOptions.cpp:104-121`: MakeChain reads TESTopicInfo.dataInfo/responses.head, TESResponse.data.responseID, TESTopicInfo.parentTopic+20 then topic form ID. Same INFO layout prerequisites as spoken responses. Captures max8 links, 4 buttons, 4 contexts/button; response enum0..3 and XDI vanilla order `[3,0,1,2]` (`40-50`) are behavior assumptions, not pointer offsets.
- `src/DialogueOptions.cpp:194-249`: wheel reads `BSStringT.empty`, replaces via Set; uint16 text-length limit. This is independent of GFx/XDI layout once INFO capture and BSString ABI are proven.
- `src/DialogueOptions.cpp:255-275,278-331,402-465`: XDI uses raw Value mirror, GetElement, Value type/number/string/managed reference operations; reads ActionScript `optionID`, writes `prompt`/`response`; accepts an ordered subset of the four vanilla IDs. ActionScript schema and exact menu URL/code-object paths are compatibility dependencies (`49-57,526-574`), not addresses.
- `src/DialogueOptions.cpp:335-377`: non-vanilla XDI walks PlayerCharacter singleton -> virtual GetCurrentScene **slot5C** (`B/TESObjectREFRs.h:560`) -> BGSScene.actions+90 and currentActivePhase+CC (`B/TESForms.h:2925,2931`) -> BGSSceneAction.GetActionType **slot08**, status+14, startPhase u16+0C, endPhase u16+0E (`B/BGSSceneAction.h:86,103-106`) -> cast to BGSSceneActionPlayerDialogue -> inherited responseTopics[4]+20 (`140`) -> TESTopic.topicInfos+50 / numTopicInfos+60 (`B/TESForms.h:1664-1666`) -> INFO chain. Enum kPlayerDialogue=3 and Status::kRunning=1 are also ABI semantics. This traversal can be gated separately from captured vanilla-layout XDI, but must not guess non-vanilla identity.
- `src/DialogueOptions.cpp:467-481`: derived GFx::FunctionHandler overrides **Call slot01**. Params reads retVal+00, self+10, args+20, argCount u32+28 (`G/GFx_Player.h:1034-1050`, Params size38; handler size10). It forwards original method, then patches return Value while active.
- `src/DialogueOptions.cpp:508-524,533-563`: late registration reads marker/wrapper via ObjectInterface.GetMember and writes via stored original SetMember. `g_internalSetMember` avoids self-wrapping. Immediate wrapping and late wrapping must both be disabled if Value/FunctionHandler ABI is unproven.
- Movie.GetVariable/CreateFunction use **Movie.asMovieRoot+18**, then ASMovieRootBase virtual slots **32 / 30**, respectively (`G/GFx_Player.h:1254`; `CL/src/RE/Scaleform/GFx/GFx_Player.cpp:12-25`; `G/GFx_ASMovieRootBase.h:111-113`). These are hidden vtable dependencies even though project code calls named methods.

### 4.11 ActivateText and leveled-list names

Activation (`src/ActivateText.cpp:54-60,116-155`) reads TESObjectREFR.GetObjectReference inline -> `data.objectReference`: reference.data+0C0, OBJ_REFR.objectReference+20 => complete reference+E0 (`B/TESObjectREFRs.h:324-332,756,940`). It then reads base form ID, verifies ACTI-derived type when building table, interns `BSFixedStringCS`, extracts Entry*. Context age1000ms and 32-bit tick/formID packing are plugin logic. Entry-pointer representation and raw HUD block slot are runtime-sensitive.

Leveled-list (`src/LeveledListNames.cpp:55-97`) validates TESLevItem type with As, calls original secondary-base vfunc, reads form ID through manual -68 adjustment, returns leaked table's persistent `c_str()`. No object members of TESLeveledList itself are read. `B/TESBoundObjects.h:1000-1011` declares TESLevItem primary base TESBoundObject and secondary TESLeveledList+68; `B/FormComponents.h:1857-1859` declares GetOverrideName slot0A. No local expected-vtable-target or runtime-layout check is performed before write_vfunc.

### 4.12 RuntimeText descriptions and quest logs

`src/RuntimeText.cpp:26-101` builds description component-address keys via `As<TESDescription>()` and quest log numbers via form/file APIs. It does not read TESDescription.fileOffset/chunkOffset/descriptionText; its dependency is the **component's identity/base placement** and the hooked function ABI. A change to compile-time `As` base offsets breaks lookup even without direct description-field access. Output Set depends on BSString ABI.

`src/RuntimeText.cpp:107-149` hooks read form IDs / tables and call BSString.Set; stages u16 and item u8 come from engine arguments. Disk numbering is game-independent after file identity is supplied. Tables retain pointers into TranslationStore; `Reset` before reload is required (`170-173`).

### 4.13 NameRefresh: mutation and traversal

This is not merely diagnostic: `refreshInstanceNames` defaults **true** (`include/Settings.h:20`), and `src/main.cpp:103-105` invokes it at NewGame/PostLoadGame.

| Dependency | Source access | Declared layout / evidence |
|---|---|---|
| Display-name guard/read | `src/NameRefresh.cpp:60-63,103-133` | ExtraTextDisplayData.displayName+18; inherited flags+10 bit0; displayNameText+20; ownerQuest+28. `B/BSExtraData.h:318,719-726`; size48. |
| Object mods | `src/NameRefresh.cpp:67-77` | BGSObjectInstanceExtra.values+18; GetIndexData -> BSTDataBuffer<1>.GetBuffer(0). `B/BSExtraData.h:531`; `CL/src/RE/Bethesda/BSExtraData.cpp:28-31`. ObjectIndexData.objectID u32+00, index byte04, rank05, disabled06; stride08 (`B/BGSMod.h:122-130`). Packed shifts32/40/48 are comparison logic. |
| Instance-data ownership | `src/NameRefresh.cpp:144-170` | ExtraInstanceData.data smart pointer+20, copied to retain TBO_InstanceData across repair; size28 (`B/BSExtraData.h:456-470`). Smart-pointer refcount/vtable deletion ABI matters. |
| Name rebuild call | `src/NameRefresh.cpp:155-158` | ExtraDataList.SetDisplayNameFromInstanceData(457340/2190179). May delete text extra, so re-reads afterwards; this has destructive mutation consequences. |
| Placed reference | `src/NameRefresh.cpp:252-257` | TESObjectREFR.extraList+100 and base getter+E0; inherited form ID. |
| Container/player inventory | `src/NameRefresh.cpp:197-227,263-277` | ref/player.inventoryList+F8 -> BGSInventoryList.rwLock+78, data BSTArray+58 -> BGSInventoryItem.object+00, stackData+08 -> Stack.nextStack+10, extra+18; item size10, stack size28. `B/TESObjectREFRs.h:410-416,942`; `B/BGSInventoryItem.h:45-50,205-209`. |
| Interior cells | `src/NameRefresh.cpp:237-260,340-342` | TESDataHandler.interiorCells+F58 -> TESObjectCELL.spinLock+C0 and references+70, copied as NiPointer<TESObjectREFR> while locked. `B/TESForms.h:1578,1581`. NiPointer copy increments BSHandleRefObject/NiRefObject refcount. |
| Exterior cells | `src/NameRefresh.cpp:347-367` | TESDataHandler formArrays -> TESWorldSpace.persistentCell+70, cellMap+40. `B/TESWorldSpace.h:126-127`; size2D0. Deduplication is plugin logic, not layout protection. |
| Detail-only probe | `src/NameRefresh.cpp:280-308,324-327` | LookupForm(0x002872, "Tull Collection.esp"), extraList, extra types and base FULL; raw 16-byte prologue read from single ID457340. Specific mod form is a data identifier, not an Address Library ID. |

The comments in `include/NameRefresh.h:5-8` say other inventories are only counted; live `WalkContainer` actually calls Process for Hangul names (`src/NameRefresh.cpp:224`). The source is authoritative for runtime exposure. `catch (...)` (`393-403`) is not an ABI or invalid-memory validation boundary.

### 4.14 NameDiag: read-only still requires valid layouts

`src/NameDiag.cpp:19-72,78-114,118-143,206-284,287-334` reads the same full-name / reference / text-extra / inventory / interior-cell / template layouts as above. Additional dependencies:

- `ref.parentCell+ B8`, player.parentCell; cell.IsInterior reads cellFlags+40 bit0 (`src/NameDiag.cpp:122,140,214-220`; `B/TESObjectREFRs.h:939`; `B/TESForms.h:1557,1563`).
- `ref.Get3D()` virtual **slot8C** (`src/NameDiag.cpp:139`; `B/TESObjectREFRs.h:604`), no load/build function called.
- Iterates numeric extra types0..FF in holster trace (`src/NameDiag.cpp:132-136`); GetByType must remain safe for those enum values. Requires enum values for MapMarker/ObjectInstance/TextDisplayData/InstanceData wherever typed GetByType is used elsewhere.
- `CellAttachDetachEvent.cell+00, type+08`, size10; pre/post attach/detach enum semantics. Singleton source+00, size58, engine ID862142/2192250 (`src/NameDiag.cpp:156-188,195-203`; `B/Events.h:159-192`).
- `BSTEventSink::ProcessEvent` override and event-source registration container/lock ABI. Diagnostic sink once installed continues receiving events, not only when an explicit walk is requested.
- ARMO census reads fullName, objectTemplate.items/item.fullName, instanceNamingRules and virtual TESForm.GetFormEditorID **slot3A** (`src/NameDiag.cpp:304-321`; `B/TESForms.h:700`).

`diagnoseNames=false` (default) prevents main's walks/registration. Read-only diagnostics can still crash an unsupported runtime; they must not be enabled as a compatibility probe before their prerequisites are proven.

## 5. Shared implicit engine dependencies (not visible as REL:: in project code)

### 5.1 Calls, globals, and selectors reached through RE wrappers

| Consumer / dependency | Header-declared OG / NG IDs | Declaration evidence |
|---|---|---|
| All form lookup: global form map / read lock | 422985 / 2689178; 691815 / 2689189 | `B/TESForms.h:723-731,756-767` |
| All wrong-type diagnostic strings: enum-string array | single ID1309967, array159 | `B/TESForms.h:793-815`; even error logging can resolve an OG-only ID. |
| Body/quest file ownership | 1376557 / 2193103 | `B/TESForms.h:748-754` |
| TESDataHandler singleton | 711558 / 2688883 | `B/TESDataHandler.h:95-98` |
| FULL sparse map | 226372 / 2661402, **-08** | `B/FormComponents.h:1496-1500` |
| GMST singleton | 8308 / 2690301 | `B/Settings.h:383-386` |
| NPC copy runtime cast | single ID84112; plus RTTI::BaseFormComponent and RTTI::TESForm descriptors | `CL/include/RE/RTTI.h:113-117,203-216`; this is distinct from ordinary As. |
| Player singleton | 303410 / 2690919 | `B/PlayerCharacter.h:266-270` |
| Instance name rebuild | 457340 / 2190179 | `B/BSExtraData.h:983-988` |
| Cell event singleton | 862142 / 2192250 | `B/Events.h:182-186` |
| BSStringT<char>.Set (wheel/descriptions/journal) | 407804 / 2189084 | `B/BSStringT.h:97-102` |
| Narrow string-pool GetEntry / Entry.release | 507142 / 2268729; 1204430 / 2268720 | `B/BSStringPool.h:139-143,18-22` |
| Game heap singleton / Allocate / Deallocate | 343176 / 2193197; 652767 / 2267872; 1582181 / 2267874 | `B/MemoryManager.h:197-215`; malloc/calloc/free wrappers `266-298,317-321`. Applies to GMST, pooled/container support and smart-pointer cleanup, not just explicit malloc. |
| Cell spin-lock acquisition | 1425657 / 2192245 | `B/BSLock.h:25-29` |
| Read/write lock acquisition | 1573164 / 2267897; 336186 / 2267898 | `B/BSLock.h:71-82`; unlocks manipulate local lock fields. Form lookup and GetByType take read locks; event/container operations can take write locks. |
| GFx managed Value add/release | 244786 / 2286228; 856221 / 2286229 | `G/GFx_Player.h:538-550` |
| GFx GetArraySize / GetMember | 254218 / 2285791; 1517430 / 2285936 | `G/GFx_Player.h:560-572` |
| GFx SetMember / Invoke | 1360149 / 2286589; 655847 / 2286101 | `G/GFx_Player.h:575-586`; hook and called wrapper must select same engine function. |

`As<T>()` is **not RTTI**: `B/FormUtil.h:27-46` switches formType then uses compile-time static_cast for each concrete form. Every name/description cast therefore imports the relevant class's declared base placement. A correct form tag is not proof of correct component address. `GetFormArray<T>` likewise indexes `formArrays[underlying T::FORM_ID]` (`B/TESDataHandler.h:149-157`), importing enum numbers and array stride.

### 5.2 Shared value/container/ownership layouts

- TESForm reads formID+14 and formType byte+1A (size20, `B/TESForms.h:908-915`); player identities07/14 are semantic constants (`861-862`). Every feature inherits this prerequisite.
- BSFixedString/CS holds Entry* at00 (`B/BSFixedString.h:195-212`); BGSLocalizedString wraps it at00, size08 (`298-306`). Reading c_str/length and assigning imports the pool's Entry layout: flags/refcount+08, union length/right+10, size18 and text immediately after entry (`B/BSStringPool.h:25-106`). The header's `_crc` comment also says08 although sequential u16 declaration places it after `_flags`; do not treat all comments as verified offsets.
- BSStringT<char> has data pointer then u16 size/capacity (`B/BSStringT.h:152-154`); dynamic allocator uses game calloc/free, Set is an engine call. No local sizeof assertion; do not confuse it with pooled fixed strings.
- Engine BSTArray / BSTHashMap / BSSimpleList / NiTPrimitiveArray / BSTBTree are read and sometimes mutated by compiled library templates. Their allocation, storage, element stride, sentinel/hash and iterator contracts are ABI dependencies, even where no raw constant occurs in project code. A map's relocation does not prove the container layout. Uses: sparse FULL/prompt maps; form arrays; data-handler files; message buttons; quest objectives; terminal menu/body arrays; INNR rule arrays; chargen arrays; worldspace cellMap; inventory arrays; GMST tree.
- ExtraDataList typed GetByType is inline: lock at20, BaseExtraList at08, then BaseExtraList head00/tail08/type-bitset pointer10 and BSExtraData.next08/type byte12 (`B/BSExtraData.h:791-813,905-909,949-960,1064-1067`). It depends on enum ordinals and linked-list/header layout; it is not an engine accessor that can compensate for changed offsets.
- BSSpinLock size08 (owner thread00, count04); BSReadWriteLock size08 (writer00, lock04), inline unlock semantics (`B/BSLock.h:49-65,99-120`). A wrong enclosing lock offset can corrupt memory even on an intended read-only traversal.
- NiPointer<TESObjectREFR> and BSTSmartPointer<TBO_InstanceData>/inventory stacks/extra lists access engine refcounts, potentially invoking destruction virtuals on release. NameRefresh's buffer copy and held instance data rely on these ownership contracts. Global player pointer is a NiPointer, not an untyped singleton pointer.
- GFx::Value size20, ObjectInterface size10, Movie size20, FunctionHandler size10, Params size38; enum type mask, managed bit and raw data union representation are all imported (`G/GFx_Player.h:147-179,1017-1052,1254-1256`). Project reads only objectInterface/data directly, but normal methods access the other fields and invoke reference-management engine calls.

These shared dependencies are a prerequisite set, not a suggestion to disable every feature independently after an unsafe lookup. Gate *before* first use if the prerequisite is unproven.

## 6. Disk-format and runtime-independent code coverage

No REL, RE object dereferences, hooks or vtable patches exist in `src/core/*.cpp`, `src/core/PluginFile.h`, or `include/core/*.h`. They do encode **file-format/identity** contracts which remain relevant if a new runtime changes plugin formats:

| Contract | Source |
|---|---|
| Record/group header24 bytes; signature00/size04/flags08/formID0C; subrecord header6 with u16 size+4; XXXX carries u32 next size | `src/core/PluginFile.h:22-24,44-50,98-122` |
| Compressed flag00040000; localized TES4 flag80; compressed body starts with u32 inflated size then zlib data | `src/core/PluginFile.h:23-24,79-94` |
| Own-record master-index filtering; low24-bit comparison; skip child groups; top-level type group0 | `src/core/PluginFile.h:139-195` |
| BPND type disk byte decimal65; missing-part sentinelFF; RACE NAM0+MNAM/FNAM block ordering | `src/core/ChargenLayout.cpp:11-49`; `include/core/ChargenLayout.h:19-32` |
| Quest stage INDX u16; item limited to256 positions/u8, localized CNAM u32 nonzero, QOBJ ends stage traversal | `src/core/QuestLogNumbers.cpp:11-52` |
| Full form IDs: compile index<<24; light FE + small index<<12 + lowFFF; ordinary lowFFFFFF | `src/core/TranslationStore.cpp:185-223`; `include/core/TranslationStore.h:56-62` |
| Translation precedence, name-slot/index rules, dialogue source selection | `src/core/TranslationStore.cpp` (entire); `src/core/NameSlots.cpp` (entire); `src/core/DialogueOptions.cpp` (entire). No executable layout assumptions. |

Remaining project headers are API declarations or pure helpers: `include/{ActivateText,BodyPartsChargen,ConstTranslation,Dialogue,DialogueOptions,LeveledListNames,Manager,MiscTypes,NameDiag,NameRefresh,PortedConstFields,RuntimeText,Settings,TemplateNames}.h`; `include/Hangul.h` is pure UTF-8 range detection. `include/HookUtil.h`, `include/PCH.h` and `include/TextWrite.h` carry the dependencies already catalogued. `src/Settings.cpp` only reads INI settings; defaults are in `include/Settings.h:15-24`. `updateInterval` is read but not used to install a hidden polling hook. Xbyak is included/linked but no project source emits an additional Xbyak hook.

## 7. Safe gating boundaries and required proof

**These are recommendations for a future implementation, not gates added by this inventory.** Gate before constructing a static REL relocation, scanning/dereferencing a map, installing a hook, traversing an unproven member, or interning an engine string. A post-access null/count/vptr check is too late to establish ABI safety.

| Capability / safe boundary | Where to gate | Prerequisites / retained behavior |
|---|---|---|
| Entire engine integration | Version/Query/Load, before `F4SE::Init`-dependent engine work and install calls (`src/main.cpp:126-192`) | Correct runtime classification/build selectors, supported F4SE ABI, Address Library handling, common form/string/heap/lock layouts. Unknown runtime should not be broadly admitted on the strength of IDs alone. Core file/translation rules stay independent. |
| NPC FULL copy hook | Start of `ConstTranslation::InstallHooks`, **before** vtable/target relocations (`425`) | ID, thunk signature/prologue, slot6, RTTI cast and template names. Disabling hook need not disable verified direct field passes. |
| Individual const fields | Dispatch before `IndexedField.write` (`src/ConstTranslation.cpp:345-354`) or start of each writer | Objective, message, terminal, LSCR/MGEF/NPC/REFR/REGN layouts can be individually disabled. PERK separate loop must also be gated (`357-376`). GMST separate boundary (`387`). |
| FULL own/template and INNR | `ApplyNames` before lookup/cast/Inspect; hook FindName path; `TemplateNames::ApplyInnr` before data-handler iteration | Template-aware own-name indexing reads template arrays even for own-name requests. INNR can be disabled separately. Do not bypass templates by guessing indices. |
| Misc individual types | Per `kFields` dispatch (`src/MiscTypes.cpp:117-140`) | Especially AVIF: skip unsupported +160 writer before raw access. Other verified types need not be disabled. |
| Body part and each chargen type | `BodyPartsChargen::Apply`/before `ApplyBodyParts` or each `ApplyRace` slot builder (`317-324`) | Avoid unproven pointer traversal before count check. BPTD, TTGP, MPPN, FMRN separable if common form/file prerequisites are proven. |
| Spoken responses | `Dialogue::Install` before ID755245 | Keep INFO NAM1 hook off if constructor or INFO/TESResponse layout unproven. |
| Prompt-map patching | `Dialogue::ApplyPrompts` before `FindPromptMap` (`119-121`) | Independent from spoken hook. Require unique pattern hit, expected instruction bytes, signed displacement/target validation and proven map representation. No speculative scan-and-write fallback. |
| Description / journal hooks | Each call in `RuntimeText::Install` (`158-159`), with corresponding Apply builder | Can separate if description-component identity is proven but quest ABI is not, or vice versa. Preserve Reset lifetime boundary. |
| Vanilla wheel | Before context and button installations (`src/DialogueOptions.cpp:588-589`) and before activating an unproven path | Context capture can remain for **verified** vanilla-layout XDI if button fails. Conversely button without usable capture does not supply a safe translation identity. Current logging alone does not enforce a complete capability predicate. |
| XDI wrapper | Before SetMember hook AND Scaleform callback registration (`594-603`), and before WrapNow | Requires Value/Params/FunctionHandler/Movie/root-vtable/engine-call contracts and ID827659. A no-op `g_active` flag does not stop wrapper creation, original invocation, or registration monitoring. |
| XDI active scene traversal | Before `CollectActiveInfos` (`src/DialogueOptions.cpp:415-417`) | If scene layout unproven, skip non-vanilla option rewriting rather than infer a mapping. Captured vanilla-layout XDI can remain if its independent contracts are proven. |
| Activation prompt | Start of `ActivateText::Install`, plus Apply if string ABI unproven (`106,116`) | Treat context/HUD pair as one capability: require both addresses/signatures and primary-slot ABI. Current code independently attempts both and ignores success values; no transactional install. |
| LVLI ONAM | Start of `LeveledListNames::Apply`, before data build/first vtable lookup (`55`) | Prove secondary-vtable ID, slot0A and -68 adjustment. Gate before `g_installed` changes; cannot undo a bad installed hook by emptying translations. |
| Instance-name repair | Before `NameRefresh::Apply` in main, or before its Run (`src/main.cpp:103-105`; `src/NameRefresh.cpp:393-397`) | Default-on mutation is highest consequence. Require entire traversed reference/cell/world/inventory/extra/instance chain and repair signature. Skip all unsupported repair; do not rely on exception handlers or “unchanged mods” logging. Can separately disable detail raw-ID probe. |
| Name diagnostics / sink | All NameDiag call sites and registration (`src/main.cpp:61-63,82-85,94-98,106-109,113-118`) | `Install` gate alone is insufficient: GDRstart and other explicit walks precede/omit sink registration. Keep off until read layouts/event ABI proven. |

### Bounded gaps for the support table

- **OG 1.10.163:** existing implementation/source assumptions only in this investigation; prior in-game claims are identified above, not reproduced. Do not turn this inventory into a fresh test-pass claim.
- **NG 1.10.980 / .984 and every listed AE runtime:** all translation capabilities **UNVERIFIED / disabled** under the admission policy in `src/Runtime.h`. Successful compilation establishes neither hook nor layout support.
- **Address resolution:** no bins decoded; no executable verification; no NG executable available. IDs in this document are source-declared mappings, not runtime-specific resolved addresses.
- **Transitive breadth:** directly named project hooks/relocations/raw layouts are catalogued; important shared wrappers are traced. This is **not an exhaustive enumeration of every instantiated SDK template's allocator, refcount, destructor, RTTI descriptor, container-internal or event-dispatch relocation**. Those ownership/container contracts are explicitly prerequisites, not implicitly supported capabilities. Further expansion of transitive IDs is deferred; it does not change the disabled NG/AE decision.
- **Concurrent changes:** line references and observations describe the inspected snapshot. Earlier feature line references predate the `src/Runtime.h` integration; the final spot-check of its load/listener guards is recorded in section 1. Reconcile other moved `main.cpp` lines before publishing a final support table.

### Verification work still required

1. Resolve every directly named ID/vtable ID and the shared wrapper IDs on each target bin. Record absent mappings; do not call a failing lookup merely to see whether a feature can be disabled afterward.
2. At each available executable target, identify function signature, complete safe prologue bytes and behavior. No NG executable was available: bin evidence alone cannot establish signatures/layouts.
3. Prove actual engine member accesses, base placements, container strides and virtual slots per supported version. Header comments/assertions and successful compilation are not such proof. Pay special attention to AVIF, quest-objective mirror, perk raw layouts, activation parameter block, XDI active-scene traversal, and default-on name repair.
4. Validate prompt-scan uniqueness and operand decoding; no current uniqueness assertion exists.
5. Verify Address Library file naming/format loading and runtime detection separately from feature validation. This document did not inspect bins, patch CommonLib, or certify loader behavior on AE.
6. Preserve existing assertions/tests. No tests were changed or run here; the 35-pass/2-skipped baseline was not reproduced. **In-game status for every item remains UNVERIFIED.**
