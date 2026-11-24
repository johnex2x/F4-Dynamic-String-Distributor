# AE independent location results (Phase A, step 1)

Completion time: see the last line of this file (shell clock stamp taken right after writing).

Scope: NG (1.10.98x) is not supported. OG and AE only. No NG column.
Analysis only. Scripts and caches live in `ae/` (local, not published): `lib.py` (bin loader, PE reader, Capstone, RTTI, xrefs, anchors), `notes.md` (raw running notes).
Executables used: 1.11.191 `Fallout4.exe`, 1.11.221 `Fallout4.exe`, 1.11.240 `Fallout4.exe`. The OG exe `Fallout4.exe` was used ONLY for its readable `.pdata` (function start and size). Its `.text` bytes are encrypted and were never used as evidence. AE 1.11.137/.159/.169 have no exe: their RVAs below are Address Library lookups only (bytes unverified).

## Method

1. **Order-preserving anchors.** 233 `RelocationID(og, ng)` pairs in the `extern/CommonLibF4` headers resolve in the OG bin and in the AE 240 bin to `.text` RVAs. The new ID space (2xxxxxx) is assigned in OG address order, and function order inside the exe is kept. So each OG target is bracketed by two known pairs, which gives an ID window and an AE RVA window (`lib.predict`, `lib.cands`). Observed prediction error: GetActivateText off by 2 IDs.
2. **Size filter.** OG function size from OG `.pdata`: GetActivateText 0xB0, ShowRollover 0x255, CopyComponent 0x7D, response ctor 0x47, INFO capture 0x245, wheel 0x65, GetElement 0x7B, REGN Find 0x63, description 0x58, journal 0x10C, name probe 0x50, SetMember 0x51F. AE sizes are expected to be similar.
3. **Shape check.** Each candidate in a window was read in Capstone and compared with the OG contract in DSD source (arguments, member offsets, helper calls). Strings, RTTI and vtables were used where they exist.
4. **Cross-version.** For each found function the AE ID was looked up in all six 1.11.x bins; the RVA of 191/221/240 was converted back to an ID (all return the same ID). Function bytes (from `.pdata` bounds) were compared after masking only RIP-relative displacements and rel32 call/jmp targets ("normalized-same"). Raw bytes differ between builds only through those displacements (raw hash identical only for REGN Find and the name probe).

## Direct functions

AE RVA order: 137 / 159 / 169 (bin only, no exe) / 191 / 221 / 240. All six lookups returned the stated ID; reverse RVA to ID from each bin gives the same ID.

| Target | OG ID | AE ID | RVAs 137 / 159 / 169 / 191 / 221 / 240 | Size 191=221=240 (OG) | 191/221/240 normalized-same |
|---|---|---|---|---|---|
| HUDRollover ShowRollover | 414044 | 2221994 | A19920 / A19A60 / A19E50 / A1C880 / A1C8C0 / A1CBD0 | 580 (OG 597) | yes |
| TESObjectREFR::GetActivateText | 1077221 | 2201128 | 510630 / 510770 / 510B60 / 513590 / 5135A0 / 5138C0 | 176 (OG 176) | yes |
| TESFullName CopyComponent (vtable 487988 slot 6) | 1510422 | 2193213 | 312C30 / 312D70 / 313160 / 315B90 / 315BA0 / 315EC0 | 125 (OG 125) | yes |
| GetCurrentTopicInfo (INFO capture) | 781358 | 2196825 | 413370 / 4134B0 / 4138A0 / 4162D0 / 4162E0 / 416600 | 564 (OG 581) | yes |
| DialogueMenuUtils SetButtonText (wheel) | 643840 | 2249662 | 107E9A0 / 107EF60 / 107F570 / 1083A10 / 1083BB0 / 1083F00 | 245 (OG 101) | yes |
| ObjectInterface GetElement (XDI) | 827659 | 2285881 | 1AC1790 / 1AC1E80 / 1AC2800 / 1AC6F40 / 1AC7060 / 1AC7520 | 123 (OG 123) | yes |
| TESRegionDataList Find (REGN) | 657534 | 2196228 | 3EA440 / 3EA580 / 3EA970 / 3ED3A0 / 3ED3B0 / 3ED6D0 | 99 (OG 99) | yes (raw identical) |
| Name rebuild / repair probe | 457340 (pair 2190179) | 2190179 | 27B680 / 27B7C0 / 27BBB0 / 27E5E0 / 27E5E0 / 27E900 | 85 (OG 80) | yes (raw identical) |
| ObjectInterface SetMember (XDI/GFx) | 1360149 (pair 2286589) | 2286589 | 1AE1170 / 1AE1860 / 1AE21E0 / 1AE6920 / 1AE6A40 / 1AE6F00 | 1403 (OG 1311) | yes |
| LVLI GetOverrideName (TESLevItem 2nd vtable 1381610, slot 0xA) | vtable slot | 2198199 | 44E240 / 44E380 / 44E770 / 4511A0 / 4511B0 / 4514D0 | 168 | yes |

### Evidence per row

- **ShowRollover 2221994** (240 RVA 0xA1CBD0). Found from the string `AdjustRolloverPositions` (240 0x2533300, xref at 0xA1CDF0 inside this function; string RVA 0x252B290 in 191 and 221). The function takes (this=HUDRollover, rdx=params). It reads three BSFixedString slots of the params: `[params+0]` goes to the widget at `this+0xF8/+0x100` (noun), `[params+8]` to the widget at `this+0x3E0` (primary action), `[params+0x10]` to the widget at `this+0x478` (secondary action), and flag bytes at `+0x18..+0x1C`. This proves `ShowRolloverParameters +0x08 = primary action BSFixedString` on 191/221/240. Its only caller is 0xA1C960 (ID 2221993, the HUDRollover update, which keeps a params copy at `this+0x518`). The HUDRollover constructor 2221992 (which also references `RolloverName_tf`) was rejected: it is a constructor, not a params consumer.
- **GetActivateText 2201128** (240 0x5138C0). Found in the anchor window OG 0x40B760 / 0x40EFF0 = AE 240 0x513630 / 0x517030 (IDs 2201126 / 2201163). Body: `rdi=[this+0x100]` (extra data list), FindExtra type `0xC4`, which equals `kActivateText` (index 196 in the `BSExtraData.h` enum). If the extra exists, it formats the text into the out string (rbp=rdx). Otherwise `rcx=[this+0xE0]` (base object), `call [vtbl+0x300](this, out)` (`TESBoundObjects::GetActivateText`, slot 0x60), then `al=1`. Same size as OG (0xB0). Signature `bool(TESObjectREFR*, BSString&)` matches.
- **CopyComponent 2193213.** `TESFullName` vtable (ID 487988, RTTI `.?AVTESFullName@@` confirmed in all three exes) slot 6: 191 0x315B90, 221 0x315BA0, 240 0x315EC0. Body: RTTI cast of rdx (`call 0x1422C46AE` with two type descriptors), then BSFixedString assign `this+8 = src+8`. This proves the TESFullName string at component+8 and slot number 6. Same size as OG (0x7D).
- **GetCurrentTopicInfo 2196825** (240 0x416600). Found in window ids 2195843..2197003 by size and shape: `rcx=action ([rcx+8] index)`, `rdx=scene` (hash lookup through `[scene+0xA8]`), `r8=target`, `r9d=responseType`. When responseType is 4 it substitutes `[action+0xE0]`. A near sibling, ID 2196828 (0x4168E0, size 0x231), has a different argument shape (rcx, dl, r8, r9) and was NOT chosen. Confidence: medium-high (shape match, no string anchor).
- **SetButtonText 2249662** (240 0x1083F00). Found through the string `DialogueSetButtonText` (240 0x271DC60), which the function fetches through static getter 0x1A869E0 (ID 2284851). Arguments: `cl=button` (checked `<4`), `rdx=BSString*` (text pointer is `[rdx]`; null falls back to a default string), `r8b=challengeLevel` (checked `<8`), `r9b=enabled`. It builds a UI message: text at `msg+0x20`, button byte `+0x18`, challenge `+0x38`, enabled `+0x39`, then sends it. Size mismatch: AE 245 vs OG 101 bytes. Confidence: medium-high (semantics and argument checks match, size does not). The other user of the getter, 0x10434BD (size 0x1F8, string `SetButtonText`), is a different function.
- **GetElement 2285881** (240 0x1AC7520). Window ids 2285873..2286101, size 0x7B equals OG. `rcx=ObjectInterface`, `rdx=array data` (`[rdx+0x40]` element count compared with `r8d` index; out of range returns false), `r9=Value*` out (if bit 6 of `[r9+8]` is set, the old managed value is released; then `[r9+8]=0`). The element is read through `[this+8]->[+0x18]`. This also shows the Value layout from AE: type dword at +8 with managed bit 0x40.
- **REGN Find 2196228** (240 0x3ED6D0). Size 99 equals OG 0x63; raw bytes identical in all three exes. Body: walk of a singly linked list (node[0]=data, node[8]=next), `call [data vtbl+0x18]` (vtable slot 3, `GetID`), compare with `edx`, return the data pointer or null. Note: `src/PortedConstFields.cpp` already cites 2196228; this analysis rederived it from shape and size, so it adds confirmation, not new information.
- **Name repair probe 2190179 and SetMember 2286589.** Both are CommonLibF4 `RelocationID` pairs (457340/2190179, 1360149/2286589), so identity rests on the pair; each AE function was additionally checked. 2190179: first work is `FindExtra(this, 0x99)`, and 0x99 is `kTextDisplayData` (index 153) in the same enum, consistent with a display-name rebuild; size 0x55 vs OG 0x50. 2286589: ObjectInterface method (rcx=this, rdx=data, r8=name, r9=Value*, flag), prologue reads `[this+8]->[+0x18]` like GetElement; size 0x57B vs OG 0x51F. Both instruction-identical in 191/221/240.
- **LVLI GetOverrideName 2198199.** `TESLevItem` secondary vtable ID 1381610: RTTI `.?AVTESLevItem@@` with COL offset 104 (0x68) in 191/221/240, so the TESLeveledList base offset 0x68 is proven (also `add rbx, -0x68` in the function). Slot 10 (0xA) = 191 0x4511A0 / 221 0x4511B0 / 240 0x4514D0. Body: key = this-0x68, lookup in a global hash map (global RVA 0x2ED5FF8 in 191/221, 0x2EE0FF8 in 240; node key at +0, value BSFixedString at +8), returns the string. This is exactly `GetOverrideName`. The primary TESLevItem vtable 718716 also resolves to `.?AVTESLevItem@@` offset 0.

## Vtable identity (RTTI of the Address Library vtable IDs)

| Vtable ID | RTTI class (191, 221, 240 all) | COL offset |
|---|---|---|
| 487988 | TESFullName | 0 |
| 1381610 | TESLevItem | 104 (0x68) |
| 718716 | TESLevItem | 0 |
| 846981 | BGSEntryPointPerkEntry | 0 |
| 859152 | BGSEntryPointFunctionDataActivateChoice | 0 |

Perk entry and activate choice are used only for vptr identity comparison in DSD, so identity is what is needed; the member offsets (entry+0x18, data+0x18, data+0x28) were NOT proven.

## Explicit offsets proven from an AE accessor (191/221/240)

| Offset | Value | Evidence |
|---|---|---|
| ShowRolloverParameters primary action | +0x08 BSFixedString | ShowRollover body |
| ShowRolloverParameters secondary action | +0x10 BSFixedString | same |
| TESFullName string in component | +8 | CopyComponent body |
| TESFullName CopyComponent slot | 6 | vtable listing |
| TESObjectREFR base object | +0xE0 | GetActivateText body |
| TESObjectREFR extra data list | +0x100 | GetActivateText body |
| TESLeveledList base in TESLevItem | +0x68 | COL offset 104 and GetOverrideName |
| TESLeveledList GetOverrideName slot | 0xA of 2nd vtable | vtable listing |
| TESRegionDataList node | data +0, next +8 | Find body |
| TESRegionData GetID slot | 3 | Find body (`call [vt+0x18]`) |
| GFx Value type dword | +8, managed bit 0x40 | GetElement body |
| BSTHashMap<TESForm*,BSFixedString> node | key +0, value +8 | GetOverrideName body |
| AVIF abbreviation +0x1A0, TESForm type +0x1A | not re-derived here (earlier docs only) | `docs/runtime-layout-evidence.md` |

## Prompt table (`src/Dialogue.cpp`)

- The byte pattern `48 83 C6 18 48 83 C7 18` has exactly one hit in each of 191/221/240 (0x215041F / 0x21504FF / 0x2157F2F). In 240 the hit at 0x2157F2F is inside function 0x2157DA6 (size 0x248, no ID). It is a destructor-style loop over an array reached through `[r14+0x88]` (0x18-byte BSFixedString records), then `mov rsi,[r14+0x88]` at match+0xC. This frees a per-object array. It is not the global INFO to RNAM prompt hash map. So match+0xC is not a RIP-relative load of the map and the OG recipe does not apply.
- Where the AE map really lives: NOT FOUND. Tried: loop-shape search (12 functions with the double `+0x18` step all reference the same generic globals 0x3E75C90 / 0x3E74AB0), RNAM-immediate search (`RNAM` appears in 9 functions; none is a TESTopicInfo vtable slot or a caller of one; the INFO loader chunk 0x69C3A8 has TRDT/NAM1/TRDA but no RNAM compare, so RNAM is reached through a jump table). Needs another approach (reader side: the dialogue menu code that fetches the prompt).

## NOT FOUND / NOT DONE

| Target | OG ID | Why |
|---|---|---|
| DialogueResponse constructor | 755245 | No RTTI for DialogueResponse. Window ids 2226398..2228080 holds ~40 functions of size 0x30..0x90 with stack args. ID 2226713 (size 0x47, same as OG) was read and is the constructor of a different polymorphic object. None showed the 5-pointer plus string shape. |
| Description reader | 523613 | Window ids 2192891..2193103 (AE 240 0x30A7E0..0x3123D0). Candidates near size 0x58 (2192999, 2193000, 2193031, 2193035) read and rejected. |
| Quest journal reader | 139753 | Window ids 2207743..2207810. ID 2207744 (0x66B700, stage lookup in a hash map plus flag test) is probably a callee, not the reader; its two callers (0x59CBC0, 0x1185BF0) do not fit. |
| DialogueResponse text offset +0, size 0x40 | - | depends on the constructor |
| Perk entry +0x18, choice data +0x18/+0x28 | - | no accessor examined |
| Other section-3 offsets (QuestObjective, message, terminal, GMST, RACE, XDI scene/action, extra data flags, etc.) | - | NOT DONE (time) |
| Prompt table | - | see above |

## Completion time
2026-11-21 21:04:19
