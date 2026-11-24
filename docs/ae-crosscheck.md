# AE cross-check: independent results

Independent results: `docs/ae-independent-results.md`. Every value below comes from the AE executables
(.191/.221/.240) and the Address Library bins. NG is not supported, so there is no 1.10.984 column.

"Bytes same" means: function bytes of 191 / 221 / 240 are identical after masking only RIP-relative displacements and rel32 call/jmp targets (raw hashes differ only through those). Checked on exe bytes for 191/221/240. Address Library bins for 137/159/169 give the same ID (bytes there are unverified; no exe).

| Item | OG ID | Independent AE ID/offset + evidence | Bytes same in .191/.221/.240? |
|---|---|---|---|
| GetActivateText | 1077221 | ID 2201128; 191 0x513590 / 221 0x5135A0 / 240 0x5138C0. Extra list `[this+0x100]` FindExtra 0xC4 (kActivateText), else `[this+0xE0]` base `->vfunc 0x300(this,out)`; size 176 = OG 176 | yes (normalized) |
| ShowRollover | 414044 | ID 2221994; 191 0xA1C880 / 221 0xA1C8C0 / 240 0xA1CBD0. String `AdjustRolloverPositions`; consumes params `+0` noun, `+8` primary, `+0x10` secondary BSFixedString | yes |
| TESFullName CopyComponent (vtable 487988 slot 6) | 1510422 | ID 2193213; 191 0x315B90 / 221 0x315BA0 / 240 0x315EC0. Slot 6 of RTTI-verified TESFullName vtable; RTTI cast then `this+8 = src+8` | yes |
| DialogueResponse constructor | 755245 | NOT FOUND independently (search window ids 2226398..2228080; I filtered sizes 0x30..0x90, but the real function is 0x273 bytes; OG is 0x47) | yes (normalized) |
| INFO capture GetCurrentTopicInfo | 781358 | ID 2196825; 191 0x4162D0 / 221 0x4162E0 / 240 0x416600. rcx=action, rdx=scene, r8=target, r9d=responseType; 4 means use `[action+0xE0]` | yes |
| Wheel SetButtonText | 643840 | ID 2249662; 191 0x1083A10 / 221 0x1083BB0 / 240 0x1083F00. Message `DialogueSetButtonText`; button<4, challenge<8, enabled. Size 245 vs OG 101 | yes |
| XDI GetElement | 827659 | ID 2285881; 191 0x1AC6F40 / 221 0x1AC7060 / 240 0x1AC7520. Count `[data+0x40]`, Value managed bit 0x40 at `+8`; size 123 = OG 123 | yes |
| REGN Find | 657534 | ID 2196228; 191 0x3ED3A0 / 221 0x3ED3B0 / 240 0x3ED6D0. List walk, `vtbl+0x18` GetID; size 99 = OG 99. Note: DSD source already cites this ID | yes (raw identical) |
| Description reader | 523613 | NOT FOUND independently (window ids 2192891..2193103 read; rejected candidates; the real function is split into chunks and its head chunk is only 0x26 bytes, so my size filter missed it) | yes (head chunk, normalized) |
| Quest journal reader (GetJournalTextForStageItem) | 139753 | NOT FOUND independently | candidate: yes (normalized) |
| Name repair probe | 457340 (pair 2190179) | ID 2190179 (CommonLib pair); 191 0x27E5E0 / 221 0x27E5E0 / 240 0x27E900; first call FindExtra type 0x99 = kTextDisplayData; size 85 vs OG 80 | yes (raw identical) |
| XDI/GFx SetMember | 1360149 (pair 2286589) | ID 2286589 (CommonLib pair); 191 0x1AE6920 / 221 0x1AE6A40 / 240 0x1AE6F00; size 1403 vs OG 1311; ObjectInterface method (this, data, name, Value*, flag) | yes |
| Prompt table (`src/Dialogue.cpp` pattern + 0xC) | - | NOT FOUND. The only pattern hit on 191/221/240 (0x215041F / 0x21504FF / 0x2157F2F) is a per-object array destructor loop, not the map; match+0xC is `mov rsi,[r14+0x88]` | n/a |
| LVLI override name (TESLevItem 2nd vtable 1381610 slot 0xA, base 0x68) | slot | ID 2198199; 191 0x4511A0 / 221 0x4511B0 / 240 0x4514D0. RTTI `.?AVTESLevItem@@` COL offset 104 = 0x68; body subtracts 0x68, hash lookup, returns BSFixedString | yes |
| Perk entry vtable 846981 | - | RTTI `BGSEntryPointPerkEntry` offset 0 in 191/221/240 (vptr identity only) | n/a (vtable) |
| Activate-choice vtable 859152 | - | RTTI `BGSEntryPointFunctionDataActivateChoice` offset 0 in 191/221/240 | n/a (vtable) |
| HUD primary action offset +0x08 | - | proven from ShowRollover body on 191/221/240 | yes |
| HUD secondary action offset +0x10 | - | proven from ShowRollover body | yes |
| REGN map-data name +0x10 | - | NOT DONE (not examined) | n/a |
| AVIF abbreviation +0x1A0 | - | not re-derived in this phase (earlier `docs/runtime-layout-evidence.md` proof only) | n/a |
| Perk function-data / choice offsets, QuestObjective, message, terminal, GMST, RACE, XDI scene/action, extra-data flags, other `runtime-support.md` section 3 offsets | - | NOT DONE (time) | n/a |

## Notes

- The "bytes same" column compares 191/221/240 only; 137/159/169 are Address Library lookups with no exe.

## Phase B proofs (own evidence only)

Scripts: `ae/b1.py` .. `b5.py`. Every row below was derived from AE exe bytes (.191/.221/.240) plus the Address Library bins (.137/.159/.169 give the same ID; their bytes are unverified, no exe).

| Item | OG ID | AE ID (RVA 191 / 221 / 240) | Evidence | Class |
|---|---|---|---|---|
| `__RTDynamicCast` (fork `RTDynamicCast`, `As<>`) | 84112 | **4818455** (0x22BCB9E / 0x22BCC7E / 0x22C46AE) | The call in the TESFullName CopyComponent body (`edx=0` vfDelta, `r8/r9` type descriptors, `[rsp+20]=0`) goes to an import thunk `jmp [iat]`. The IAT slot is `VCRUNTIME140.dll!__RTDynamicCast` in all three exes (pefile import table). The fork's own ID 84112 resolves to a non-thunk OG function; on AE the fork ID would silently resolve to the next bin ID (see Risks), so this ID must replace it. | INDEPENDENT |
| `TESDataHandler` singleton pointer | 711558 | **4796135** (.data 0x30DC000 / 0x30DC080 / 0x30E7100) | The same global is loaded into `rcx` before every call to `CheckModsLoaded` (ID 2192323, 16 of 16 call sites), before 11 of 12 calls to `AddFormToDataHandler` (2192271) and 5 of 5 to `CreateReferenceAtLocation` (2192301). Both are CommonLib `TESDataHandler` methods that exist in the AE bins with the same IDs. Same ID for all three exes. | INDEPENDENT |
| `TESDataHandler::LookupModByName` (files list, filename) | - | **4476758** (0x2E65F0 / 0x2E65F0 / 0x2E6910) | Body: `lea rbx,[rcx+0xFB0]` = `BSSimpleList<TESFile*> files`; node item `[node]`, next `[node+8]`; the item is a TESFile and `lea rdx,[file+0x70]` is the filename argument of an imported case-insensitive compare. Normalized bytes equal in 191/221/240. Proves `files` at +0xFB0, node layout {item, next}, `TESFile::filename` at +0x70 on AE. | INDEPENDENT |
| `TESForm::LookupByID` (engine form lookup) | - | **2193092** (0x311850 / 0x311860 / 0x311B80) | Size 154 bytes, 1393 call sites. Takes the read lock (global ID 4796476, the shared `BSReadWriteLock` read-lock function ID 2267897), loads the map pointer (global ID 4796465), hashes the u32, probes buckets of 24 bytes (key +0, value +8, next +0x10), returns the value and unlocks. Normalized bytes equal in 191/221/240. DSD calls this function on AE instead of using the fork's `GetAllForms` globals (whose AE IDs are wrong) so no hash-map template layout has to be assumed. | INDEPENDENT |

### Struct members and tables proven in Phase B (AE .191/.221/.240)

| Member / table | Value | Evidence | Script |
|---|---|---|---|
| `TESForm::formID` | +0x14 | Papyrus native `Form.GetFormID` (registered under the string `GetFormID`; the native is `mov eax,[r8+0x14]; ret`). Same bytes in all three exes (RVA 0x11449D0 / 0x1144B70 / 0x1144EC0). | `b8.py` |
| `TESForm::formType` | +0x1A | Earlier phase, `docs/runtime-layout-evidence.md` (accessor disassembly). | - |
| Form type codes | ACTI 0x1B, FLOR 0x29, FURN 0x2A, LVLI 0x38 | The form type name table (Address Library ID 1309967, stride 0x18, `{type, "ACTI", code}`) is initialised in the exe data. Read from all three exes. Matches the fork enum. | `b10.py` |
| `TESDataHandler::files` | +0xFB0, `BSSimpleList` node {item +0, next +8} | `LookupModByName` (ID 4476758), see Phase B table. | `b3.py` |
| `TESFile::filename` | +0x70 | Same function (`lea rdx,[file+0x70]` as the compare argument). | `b3.py` |
| `TESFile::flags` bit 9 (small file) | +0x334 `>> 9 & 1` | Function containing `mov eax,[rdi+0x334]; shr eax,9; test al,1`, then `0xFE000000 | (u16 [file+0x372] << 12)`, else `u8 [file+0x370] << 24`. Unique byte pattern in each exe (RVAs 0x2F97E5 / 0x2F97F5 / 0x2F9B15). | `b6.py`, `b7.py` |
| `TESFile::compileIndex` | +0x370 (u8) | same function | `b7.py` |
| `TESFile::smallFileCompileIndex` | +0x372 (u16) | same function | `b7.py` |
| `TESObjectREFR` base object | +0xE0 (= fork `data.objectReference`, 0xC0 + 0x20) | earlier phase (GetActivateText body) | - |
| `BSFixedString` entry data | +0x18 | `BSFixedString(const char*)` wrapper at 0x167C1E0 (.240) and `lea rax,[rcx+0x18]` in its c_str use. Pool `GetEntry` is ID 2268729: `(Entry*& out, const char*, bool caseSensitive)`, hashes the string and probes the bucket table. Normalized bytes equal in 191/221/240. Shape check only (no string-ABI proof beyond this). | `b9.py` |
| `TESLevItem::GetOverrideName` return | `const char*` (c_str of the local BSFixedString) | Body of ID 2198199: copies the map value into a local BSFixedString, calls the c_str getter, destroys the local, returns the pointer. | `b11.py` |

### Phase B status of the original rows

| Item | Phase B class | AE gate |
|---|---|---|
| GetActivateText | INDEPENDENT | ON (activateText). Expected bytes `src/AeHookBytes.h`; offline check 3/3 PASS |
| ShowRollover | INDEPENDENT | ON (activateText). Same checks |
| HUD +0x08 / +0x10 | proven | +0x08 used (ON); +0x10 unused |
| TESLevItem GetOverrideName slot 0xA, base 0x68 | INDEPENDENT | ON (leveledNames). Guard: RTTI name `.?AVTESLevItem@@`, COL offset 0x68, slot in .text, function bytes match (`check_vtable.py`, `check_hookbytes.py`) |
| TESFullName CopyComponent (2193213) | INDEPENDENT-ONLY | OFF: the thunk needs `fallout_cast` to TESForm and `As<TESFullName>`. The fork's RTTI type descriptor IDs are the 1.10.98x ones (the AE bins have no IDs in the 2.68M data range). `__RTDynamicCast` itself is proven (ID 4818455) but the AE type descriptor locations are not. | **Phase C correction (2026-11-21):** the `As<>` premise was wrong. The fork `As<>` is a form-type switch plus `static_cast` (no RTTI), proven correct on AE in `phaseC-c1-rtti.md`. Only the `fallout_cast` in this hook needed a replacement (TESNPC vtable identity test). The hook is ON since phase C (`runtime-support.md`).
| DialogueResponse constructor | candidate ID 2227131 (post-hoc); `INDEPENDENT (post-hoc)` for these facts only: size 0x273, `this+0` is a BSFixedString assigned from `TESResponse::GetResponseText` (ID 2208288, the same ID the fork lists for it), object size 0x40, `TESResponse` fields `percent +0x10 (u16)`, `useEmotion +0x13`, `faceArchetype +0x14` read by the ctor match the fork layout | OFF: argument order (topic/info/speaker/quest) and `TESTopicInfo::dataInfo +0x30`, `responses +0x48` not proven; no accessor examined. Evidence is not conclusive. |
| Description reader | candidate ID 2193019 (post-hoc), head chunk shape only | OFF: needs `BSString::Set` (fork ID 2189084 is absent in the AE bins) and `As<TESDescription>` (RTTI); neither is proven | **Phase C correction (2026-11-21):** `As<TESDescription>` needs no RTTI (see above) and `BSStringT::Set` is engine ID 4471693; the reader (2193019) is ON since phase C (`phaseC-c4-runtimetext-prompts.md`).
| REGN map-data name +0x10 | NOT DONE | OFF: part of the ConstTranslation family |
| Quest journal reader | NOT FOUND (candidate ID 2207825 post-hoc, unconfirmed) | OFF |
| Prompt table | NOT FOUND (no new attempt in Phase B, see Risks in the hand-off) | OFF |
| INFO capture / wheel SetButtonText / XDI GetElement / SetMember | INDEPENDENT | OFF: the dialogue option chain also needs GFx Value layout, XDI scene/action members and `TESTopicInfo` members, none proven |
| Name repair probe 2190179 | INDEPENDENT-ONLY | OFF: NameRefresh chain (inventory, extra data, player ID absent) not proven |
| REGN Find 2196228 | INDEPENDENT | OFF: PortedConstFields chain not proven |
| Perk entry / activate-choice vtables | vptr identity only | OFF: member offsets not proven |
| AVIF ANAM +0x1A0 | proven (earlier) | OFF: the rest of MiscTypes (AMMO/ALCH/DOOR offsets) unproven |
| All other section 3 offsets (QuestObjective, message, terminal, GMST, RACE, XDI, extra-data flags) | NOT DONE | OFF |

### Library defects found in Phase B

- `REL::IDDB::id2offset` (`extern/CommonLibF4/CommonLibF4/src/REL/IDDB.cpp:117-150`) fails only when the ID is greater than every ID in the bin; in non-VR builds a missing ID silently resolves to the **next larger ID's offset**. Every fork wrapper whose AE ID is missing from the bins therefore returns a wrong address, not an error. The .240 bin contains no ID in 2688800..2691000 at all (the 1.10.98x data ID range). So no fork wrapper with an NG-slot ID in that range may run on AE; DSD bypasses `TESForm::GetFormByID` and `TESDataHandler::GetSingleton` through `DSD::LookupFormByID` and `DSD::GetDataHandler` (`src/Runtime.h`).
