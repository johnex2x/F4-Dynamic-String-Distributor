STATUS: PARTIAL

# Phase C2: constTranslation members and IDs on AE 1.11.x

Method: header diff fork vs CommonLibF4AE (CLAE), disassembly of AE .191/.221/.240 where needed (`phaseC/c2/h.py verify()`:
normalized disassembly equality across the three exes), ID membership across all six bins (`q24.py`).
CLAE is not independent AE proof (its AVIF abbreviation offset is the OG value +0x160, AE is +0x1A0), so members were checked in the AE exes.
Scratch evidence: `phaseC/c2/` (`q1`..`q24`, `02-verify.py`, `hdiff.py`).
Limitation: the per-offset evidence list below is a summary. The full disassembly excerpts are in the scratch dumps. The pass/fail tally of `verify()` ranges was not counted per item. The final run of `02-verify.py` showed no failing range.

## ID membership (exact in all six bins 137/159/169/191/221/240)

| ID | Use | Present in all six | Note |
|---|---|---|---|
| 2193213 | TESFullName CopyComponent (vtable slot 6) | yes | 240 RVA 0x315ec0 |
| 487988 | TESFullName vtable | yes | |
| 2194235 | BGSLocalizedString::Load | yes | |
| 2193092 | `DSD::LookupFormByID` bypass | yes | |
| 4796135 | `DSD::GetDataHandler` | yes | |
| 2192280 | formArrays accessor (INNR `GetFormArray`) | yes | |
| 2661402 | sparse FULL map object | yes (data) | object identity not proven, see group S |
| 2193216 | engine sparse-map lookup | yes | used for layout proof only |
| 2267895 | CRC helper | yes | |
| 846981, 859152 | BGSEntryPointPerkEntry / ActivateChoice vtables | yes | |
| 1510422 | fork FullNameCopyComponent slot-1 ID | ABSENT in all six | do not use; AE ID is 2193213 |
| 2193197 / 343176 | fork MemoryManager singleton | ABSENT | AE replacement 4471522 (present in all six) |
| 2690301 / 8308 | fork GameSettingCollection singleton | ABSENT | AE replacement 4797590 (present in all six) |
| 657534 | DSD OG REGN FindRegionData | ABSENT | AE replacement 2196228 (present in all six) |

## Table (DIFFERS and group-level rows; member rows in details)

| Group | Member or ID | Fork/OG value | AE value | Evidence | Class |
|---|---|---|---|---|---|
| GMST | GameSettingCollection singleton | ID 8308 / 2690301 | 4797590 | ID bin check; used by singleton accessors | DIFFERS-PROVEN |
| GMST / allocs | MemoryManager singleton (RE::malloc, GameHeapCopy) | ID 343176 / 2193197 | 4471522 | ID bin check; CLAE agrees | DIFFERS-PROVEN |
| REGN | FindRegionData | DSD OG ID 657534 | 2196228 | ID bin check | DIFFERS-PROVEN |
| FULL (all) | CopyComponent hook | 1510422 | 2193213 | already proven; fork ID absent | DIFFERS-PROVEN |
| QUST objective | mirror struct 0x28, text field | fork layout | same | AE QUST load, vtable slot 9, subrecord NNAM compare | AGREE+SPOT |
| MESG | description, button text, shortName | fork | same | MESG load slot 9 | AGREE+SPOT |
| TERM | body text, menu item text | fork | same | TERM load slot 9 | AGREE+SPOT |
| LSCR | +0x30 | fork | same | LSCR load | AGREE+SPOT |
| MGEF | +0x198 | fork | same | MGEF load | AGREE+SPOT |
| NPC_ | shortName +0x298 | fork | same | NPC load | AGREE+SPOT |
| PERK | entry funcdata +0x18, choice label +0x18, index +0x28 | fork | same | PERK entry load | AGREE+SPOT |
| REGN | mapName +0x10 | fork | same | REGN load | AGREE+SPOT |
| REFR | extraList -> ExtraMapMarker -> fullName | fork | same | BaseExtraList::GetByType, N=27 bytes | AGREE+SPOT |
| Object templates / INNR | objectTemplate.items, ruleSets, rule.text | fork | same | formArrays accessor 2192280, INNR load | AGREE+SPOT |
| GMST | Setting / GameSettingCollection tree layout | fork | same | BSTBTree: entries[4] x16, nodes at +0x40, usedEntries +0x68, root +0x10 | AGREE+SPOT |
| Strings | BSStringPool entry (flags +8, crc +0xA, length +0x10, data +0x18); "<ID=" prefix length 13 | fork | same | string pool code in AE | AGREE+SPOT |
| Containers | BSTArray (data +0, cap +8, size +0x10), BSSimpleList {item +0, next +8} | fork | same | AE accessors | AGREE+SPOT |
| Sparse FULL | BSTHashMap<TESForm*, ...> layout | fork inline find | entries at base+0x28, 0x18-byte entries {key,value,next}, mask from capacity-1, CRC over key, sentinel | engine lookup 2193216 (240 RVA 0x316000) | UNPROVEN |

## Group verdicts

| Group | Verdict | Missing |
|---|---|---|
| FULL via TESFullName component (items etc.) | BLOCKED | needs c1 RTTI casts (`As<TESFullName>`, `fallout_cast`); hook ID 2193213 itself is proven |
| FULL via sparse map (STAT/MSTT/SCOL) | BLOCKED | sparse map hash/capacity field/object identity not proven; keep OFF |
| DESC / description | BLOCKED | depends on RTTI casts (c1) |
| QUST objectives | BLOCKED | members READY; needs `As<>` for form type (c1) |
| MESG | BLOCKED | members READY; RTTI cast (c1) |
| TERM | BLOCKED | members READY; RTTI cast (c1) |
| GMST | BLOCKED | needs substitute IDs 4797590 (collection) and 4471522 (heap); then READY |
| LSCR, MGEF, NPC_ shortName | BLOCKED | members READY; RTTI cast (c1) |
| REGN mapName / region find | BLOCKED | needs ID 2196228 in place of 657534 |
| REFR map marker | BLOCKED | members READY; cast (c1) |
| PERK entries | BLOCKED | members READY; vtable identity uses VTABLE IDs 846981/859152 (present); `RE::VTABLE::...[0]` must resolve to the AE RTTI vtable |
| Object templates / INNR | BLOCKED | members READY; `GetFormArray` cast and form-type code (c1) |

Every group that is "members READY" turns READY once c1 supplies the RTTI replacement and the runtime applies the substitute IDs above
at load (resolve once, no per-call cost). `DSD::LookupFormByID` (2193092) replaces `TESForm::GetFormByID` calls.

## Counts

PROVEN/DIFFERS-PROVEN/AGREE+SPOT rows above: 4 DIFFERS-PROVEN, 13 AGREE+SPOT, 0 CLAE-ONLY, 1 UNPROVEN (sparse FULL map).
Spot-check pass/fail: all `verify()` ranges passed on 191/221/240; no failure recorded. Exact per-item count was not tallied.
TESForm::IsPlayer / IsPlayerRef was not checked (open).
