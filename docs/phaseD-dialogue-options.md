# Phase D: dialogueOptions on AE (vanilla wheel ON, XDI ON when XDI.dll is loaded)

STATUS: PARTIAL (in-game result UNVERIFIED). The vanilla wheel (GetCurrentTopicInfo + SetButtonText) is ON on AE. The XDI sub-path is ON on AE too, but its SetMember hook and Scaleform callback are installed only when `GetModuleHandleW(L"XDI.dll")` is non-null. An AE XDI build exists for 1.11.240 only; every value below is proven in .191/.221/.240 from our own executables.
(XDI v1.36 is an OG-only plugin; its AE values are not proven). In-game result: UNVERIFIED.

| sub-path | AE | reason |
|---|---|---|
| wheel context (GetCurrentTopicInfo, OG 781358 -> AE 2196825) | ON | proven below, guarded by AeHookBytes::kGetCurrentTopicInfo |
| wheel button (SetButtonText, OG 643840 -> AE 2249662) | ON | proven below, guarded by kSetButtonText and kBSStringSet |
| XDI (SetMember hook 2286589, GFx Value calls, scene walk) | ON, gated on XDI.dll | proven below (XDI proofs). Hook guarded by AeHookBytes::kGFxSetMember. Decided once in `DialogueOptions::InstallXdi`, called at F4SE kPostLoad (every plugin is loaded by then; at F4SEPlugin_Load the order is alphabetical, so XDI.dll after DynamicStringDistributor.dll would be missed). No XDI.dll -> no SetMember hook (it sits on every Scaleform member write) and no Scaleform callback. |

## IDs (exact members of all six 1.11.x bins: 137, 159, 169, 191, 221, 240)
2196825 (RVA .191/.221/.240 = 0x4162d0/0x4162e0/0x416600), 2249662 (0x1083a10/0x1083bb0/0x1083f00), 4471693 (BSStringT::Set, 0x1e61d0/0x1e61d0/0x1e64f0). 3 checked, 0 missing.
Fork wrapper with a wrong AE ID that the wheel reaches: `BSStringT::Set` (2189084 absent). Fixed with the DSD helper `DSD::BSStringSet` (src/Runtime.h, ID 4471693). The fork was not patched.
Fork wrappers with absent AE IDs that the XDI code reached, now replaced by DSD helpers: `Value::GetMember` (fork 2285936, absent) -> `GetMemberRaw` in DialogueOptions.cpp, AE ID 4494126 (OG keeps 1517430); `PlayerCharacter::GetSingleton` (fork 2690919, absent) -> `DSD::GetPlayer` in Runtime.h, AE data ID 2698073 (OG keeps the fork call). GetElement: AE 2285881 (OG 827659), chosen once. After the build, 2285936 is no longer in the DLL; 2690919 remains only in NameDiag/NameRefresh code (features OFF on AE).
XDI IDs (exact members of all six bins, 0 missing of 8): 2285881, 4494126, 2285791, 2286101, 2286228, 2286229, 2286589 (RVA .191/.221/.240 = 0x1ae6920/0x1ae6a40/0x1ae6f00), 2698073. Wrapper IDs used as evidence only: 4472734/4472735/4472736.
Avoid list (Floating Subtitles hooks 2249542, 2249545, 2229076): no new hook targets any of them.

## Evidence (disassembly, normalized function bodies identical in .191/.221/.240 for all three IDs)
- GetCurrentTopicInfo(action rcx, scene rdx, target r8, responseType r9d) -> TESTopicInfo* (rax). Body: if type==4 use `[action+0xE0]` (playerInput); flags `[action+0x10]`; responseTopics `[action+type*8+0x20]`, pNPCResponseTopics `+0x98`, keywords `+0x40/+0xB8`; then TESTopic walk (0x699610): `topicInfos [topic+0x50]`, `numTopicInfos [topic+0x60]` (loop in 0x699070), infoTree `+0x58`, ownerQuest `+0x40`; TESTopicInfo `resetGlobal +0x28`, flags `+0x44`. Caller at .240 0x415a21 stores `[rax+0x20]` (parentTopic) into action `+0xD8` (pTopic) and `+0x88` (currentTopic): proves parentTopic +0x20 and the return type.
- SetButtonText(button cl, BSStringT& rdx, challenge r8b (<8), enabled r9b); button <4 checked; `[text+0]` is the char data pointer; it calls BSString::Set (0x1e64f0 in .240 = ID 4471693). The only caller (.240 0x416ab5) passes a local BSString and frees it afterwards.
- BSStringT layout (+0 data, +8 u16 size, +0xA u16 capacity), TESTopicInfo dataInfo +0x30 / responses +0x48 / responseID: phase C4 (docs/phaseC-c4-runtimetext-prompts.md).
- Prologue bytes of both hooks: `ae/check_hookbytes.py` ALL PASS, HookPrologue patch length 5 in .191/.221/.240 (no hook of this phase is relative within the first 5 bytes).
- XDI proofs (.191/.221/.240; function bodies normalized-equal unless noted):
  - Value layout: out-of-line wrappers 4472734 (GetMember), 4472735 (SetMember), 4472736 (Invoke): interface `[rcx]`, type `[rcx+8]` masked 0x8F, data `[rcx+0x10]`, kDisplayObject = 10 passed as the last argument; managed string = type bit 0x40 and `*(const char**)data` (engine inline at .240 0xA7A9B4), as the fork GetString reads it.
  - ObjectInterface::GetMember 4494126 (rcx interface, rdx data, r8 name, r9 out, stack bool isDisplayObject): matches the fork signature; reached through wrapper 4472734.
  - GetElement 2285881 (rcx interface, rdx array data, r8d index, r9 out Value*; size at `[data+0x40]` as GetArraySize 2285791 `mov eax,[rdx+0x40]`). AddRef 2286228 / Release 2286229 (rdx Value, r8 object; type & 0x8F, refcount `[obj+0x20]`). Invoke 2286101 (equal in all three).
  - Movie asMovieRoot at Movie+0x18; the AS3 MovieRoot has no RTTI. Its vtable is found in all three exes (vtable 0x2719210 / 0x2719220 / 0x2721330, 0x46 slots, pair of the 0xAD-slot vtable). Slot 0x30 (CreateFunction, 0x1aae3e0/0x1aae500/0x1aae9c0) and slot 0x32 (GetVariable, 0x1acce20/0x1accf40/0x1acd400) are normalized-equal in all three and match the fork ASMovieRootBase slots 0x30/0x32. FunctionHandler Call is vtable slot 1 (call site 0x1abbb3e in .240) with the Params layout of the fork.
  - PlayerCharacter singleton, data ID 2698073 (RVA 0x32d2260/0x32d22e0/0x32dd370): the engine allocates 0xE10 bytes (= sizeof(PlayerCharacter) in the fork), runs the PlayerCharacter constructor (the function that stores the PlayerCharacter vtable) and stores the result in this slot (store sites 0xc3e460/0xc3e5f0/0xc3e980); 3976 code references in each exe.
  - GetCurrentScene = virtual slot 0x5C of the PlayerCharacter vtable (0xc5be50/0xc5bfe0/0xc5c370, ID 2229645): `[this+0x2d0]>>3 & 1` (has extra list) then ID 2200881 `rcx=[rcx+0x100]` and the extra-data lookup for type 0x92 (kScene) that returns `[extra+0x18]`; all three equal.
  - BGSScene: phases array `+0x30` indexed by currentActivePhase `[scene+0xCC]`; actions BSTArray data `[scene+0x90]`, size `[scene+0xA0]`; the loop calls the per-action function (ID 2206887, equal in all three; call sites 0x636a28/0x636a48/0x636d68) with each action pointer. The save/load routine (ID 2206858) resets `[scene+0xCC] = -1`.
  - BGSSceneAction (loader ID 2206940, equal in all three): subrecords ALID -> `+0x08` actorID, SNAM -> word `+0x0C` startPhase, ENAM -> word `+0x0E` endPhase, FNAM -> `+0x10` flags, INAM -> `+0x18` uniqueID; status byte `+0x14` tested with bits 1 (running) and 2 (complete) in ID 2206887. GetActionType = vtable slot 8; BGSSceneActionPlayerDialogue slot 8 (ID 2196813, `mov eax,3; ret` in all three) so kPlayerDialogue = 3. responseTopics `[action+0x20]` x4 and TESTopic topicInfos/numTopicInfos: wheel proofs above; TESTopicInfo dataInfo +0x30, responses.head +0x48: phase C4.
  - XDI.dll (AE build, read only, names only, no addresses or IDs taken): strings `Interface/DialogueMenu.swf`, `root.loaderInfo.url`, `root.Menu_mc.BGSCodeObj`, `GetDialogueOptions`, `optionID`, `prompt`, `response`, `WARNING: Scaleform registration failed.`, Scaleform classes `GetDialogueOptions`, `SelectDialogueOption`, `GetModSetting` ... , export `F4SEPlugin_Load`. These are the names the OG wrapper (`kDialogueMenuPath`, `kCodeObjPath`, `kGetOptionsName`) uses, so the registration contract is unchanged.

## Counts
AGREE+SPOT 8 wheel + XDI: 16 proven facts (GFx layout, GetMember/GetElement/AddRef/Release/Invoke signatures, MovieRoot slots, singleton, GetCurrentScene chain, scene/action members) / DIFFERS-PROVEN 3 (BSStringT::Set ID 2189084 -> 4471693; Value::GetMember fork 2285936 -> 4494126; PlayerCharacter singleton fork 2690919 -> 2698073) / CLAE-ONLY 0 / UNPROVEN 0. CLAE has no BGSSceneAction members (not used as evidence).
Spot checks: 8 passed, 0 failed.

## Risks
Not verified in game. Wheel button text relies on INFO identity captured by GetCurrentTopicInfo within 1500 ms (unchanged OG logic). Hot paths: SetButtonText runs only when a dialogue button is set; no per-call runtime check was added.


## XDI risks
Not verified in game (no AE XDI session was run). XDI loads after this plugin, so DialogueMenu.swf may be created before XDI registers GetDialogueOptions; the SetMember hook (one relaxed atomic load on the fast path) wraps it late, as on OG. If the XDI AE build changes how it registers the function, the options stay untranslated and the log says "waiting for XDI GetDialogueOptions". Avoid list unchanged: this phase hooks 2196825, 2249662 and 2286589 only; nothing targets 2249542, 2249545 or 2229076 (Floating Subtitles).
