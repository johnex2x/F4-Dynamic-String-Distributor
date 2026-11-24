STATUS: DONE

# Phase C3: miscTypes and bodyParts on AE (struct members, IDs)

Scope: `src/MiscTypes.cpp` (AMMO ONAM, ALCH DNAM, DOOR ONAM/CNAM, AVIF ANAM, INGR YNAM) and `src/BodyPartsChargen.cpp`
(BPTD BPTN, RACE TTGP/MPPN/FMRN). Analysis only; no source file changed.
Exes: .191 `Fallout4.exe`, .221 `Fallout4.exe`, .240 `Fallout4.exe`.
Evidence scripts and raw output: `phaseC/c3/v2/` (`prove.py` -> `prove.out`, `race_norm.py`, `formtypes.py`,
`locload.py`, `prefix.out`). Older notes in `phaseC/c3/` were used only as pointers; every value below was
re-derived in `v2/`.

## Counts

Rows 1 to 32: PROVEN 30 (AGREE+SPOT 26, PROVEN where CLAE has no value 3, no member or ID read 1),
DIFFERS 2 (row 5 AVIF, row 31 form lookup IDs), UNPROVEN 0. CommonLibF4AE spot checks: all 26 agreeing items were
checked in the exes (every item, not 1 in 5): 26 passed, 0 failed. Of the two disagreements, row 5 was decided for the
fork (CLAE carries the OG layout); row 31: neither library's form-map IDs are used, the engine lookup function is.
Scope limits (not UNPROVEN rows, see the end of the table section): .137/.159/.169 have no exes; the BPND file byte is
a file-format fact.

## Header diff result (fork 2nd slot vs CommonLibF4AE)

Compared: `TESBoundObjects.h` (TESAmmo), `MagicItems.h` (AlchemyItem), `TESBoundAnimObjects.h` (TESObjectDOOR),
`ActorValueInfo.h`, `TESForms.h` (TESForm, BGSBodyPart, BGSBodyPartData), `TESRace.h` (FaceRelatedData),
`BGSCharacterTint.h`, `BGSCharacterMorph.h`, `BSTArray.h`, `BSFixedString.h`, `BSStringPool.h`, `TESFile.h`.

- Agree: TESAmmo (shortDesc 0x178, size 0x1B0), AlchemyItem (data 0x1A8, addictionName +0x20, size 0x1E0), TESObjectDOOR
  (altOpenText 0x108, altCloseText 0x110, size 0x150), BGSBodyPartData (partArray 0x58, size 0x150), TESRace
  (faceRelatedData 0x698, FaceRelatedData size 0x68 and member order), tint template classes, morph classes, BSTArray,
  BGSLocalizedString, BSStringPool Entry and its four IDs, TESForm.formType 0x1A.
- **Differ: `ActorValueInfo`.** Fork: `abbreviation` 0x1A0, size 0x1D0 (`std::function` 0x40 each). CommonLibF4AE:
  `abbreviation` 0x160, size 0x190 (`msvc::function`). CommonLibF4AE carries the OG layout here. The exes decide for the
  fork (row 5).
- CommonLibF4AE has no named `BGSBodyPart::partName`: its `BGSBodyPart` stops at 0x78 and names the 8 bytes at 0x40
  `unk40`. That is the same slot (row 12). It has no `PART_DATA`.
- Fork wrappers with 1.10.98x-space IDs in their second slot: only `GetAllForms` (2689178 / 2689189) is reached by these two
  files. CommonLibF4AE uses 4796465 / 4796476 for the same globals; DSD does not need them because of C1.

## How "AE agrees" is shown

Every code region below is compared as normalized instruction text across .191 / .221 / .240 (rip-relative
displacements, branch/call targets and the RACE jump-table base masked; offsets, immediates, registers kept). All regions
compare equal (`prove.out`, `race_norm.out`: AMMO 188, ALCH 160, DOOR 168, AVIF 155, BPTD 204, BGSBodyPart 609, RACE 1999
instructions, helpers 33 to 181). So an offset seen in .240 is the same in .191 and .221; the RVAs are listed for each.
Subrecord tags are decoded from the compare/sub chains (e.g. `sub ecx,0x4c4d424f; sub ecx,0xf40bf5; cmp ecx,0xb` is
0x4D414E4F = `ONAM`).

The attribution rule: the engine's own load function for the record calls the localized-string loader (ID 2194235,
RVA 0x351490 / 0x3514A0 / 0x3517C0) with `rcx = this + offset` when it meets the subrecord tag. So the member at that
offset is the BGSLocalizedString that holds the subrecord text.

## Deliverable table

Columns: Group | Member or ID | Fork/OG value | AE value | Evidence | Class.
Evidence RVAs are listed .191 / .221 / .240.

### miscTypes

| # | Group | Member or ID | Fork/OG value | AE value | Evidence | Class |
|---|---|---|---|---|---|---|
| 1 | AMMO ONAM | `TESAmmo::shortDesc` | fork 0x178 = CLAE 0x178 (OG same) | 0x178 | AMMO Load (vslot 9, ID 2197859, 0x445090 / 0x4450A0 / 0x4453C0) decodes `DNAM` then `ONAM`; `lea rcx,[r15+0x178]` then locstr call at 0x4453BF / 0x4453CF / 0x4456EF. Same function fills `AMMO_DATA` at 0x160 (`lea r8,[r15+0x160]`, DNAM block writes 0x160..0x170), so `shortDesc` follows `data` (0x160 + 0x18) as the header says | AGREE+SPOT |
| 2 | ALCH DNAM | `AlchemyItem::data.addictionName` = 0x1A8 + 0x20 | fork 0x1C8 = CLAE 0x1C8 | 0x1C8 | AlchemyItem vslot 126 (subrecord handler, ID 2189476, 0x2617B0 / 0x2617B0 / 0x261AD0): `cmp r8d,0x4d414e44` (DNAM) then `add rcx,0x1c8; jmp locstr` at 0x2618ED / 0x2618ED / 0x261C0D | AGREE+SPOT |
| 3 | DOOR ONAM | `TESObjectDOOR::altOpenText` | fork 0x108 = CLAE 0x108 | 0x108 | DOOR Load (vslot 9, ID 2198687, 0x4689F0 / 0x468A00 / 0x468D20), `r14 = this`; `lea rcx,[r14+0x108]` + locstr at 0x468C8C / 0x468C9C / 0x468FBC; chain CNAM, +3 FNAM (the +0x130 flags byte), +9 = ONAM | AGREE+SPOT |
| 4 | DOOR CNAM | `TESObjectDOOR::altCloseText` | fork 0x110 = CLAE 0x110 | 0x110 | same function, `lea rcx,[r14+0x110]` + locstr at 0x468CBA / 0x468CCA / 0x468FEA (tag CNAM = 0x4D414E43) | AGREE+SPOT |
| 5 | AVIF ANAM | `ActorValueInfo::abbreviation` | fork 0x1A0; CLAE 0x160; OG 0x160 (DSD code `kOffsetOG`) | **0x1A0** | ActorValueInfo Load (vslot 9, ID 2209720, 0x6FD570 / 0x6FD5A0 / 0x6FD8B0): `cmp eax,0x4d414e41` (ANAM), `lea rcx,[r15+0x1a0]`, locstr at 0x6FD81B / 0x6FD84B / 0x6FDB5B | DIFFERS-PROVEN (exe = fork; CLAE and DSD's hard-coded 0x160 are OG) |
| 6 | all misc | form type codes AMMO 0x2C, ALCH 0x30, DOOR 0x20, AVIF 0x62 | fork = CLAE | same | form name table ID 1309967 (0x2ED0B60 / 0x2ED0B60 / 0x2EDBB60), stride 0x18, entries 44, 48, 32, 98 read in all three (`formtypes.out`); DOOR Load also checks the record type byte `cmp al,0x20` | AGREE+SPOT |
| 7 | all | `TESForm::formType` | 0x1A (fork = CLAE) | 0x1A | TESForm vslot 0x19 is `movzx eax, byte ptr [rcx+0x1a]; ret` at 0x2622B0 / 0x2622B0 / 0x2625D0 | AGREE+SPOT |
| 8 | all | `As<TESAmmo / AlchemyItem / TESObjectDOOR / ActorValueInfo>` (`MiscTypes.cpp:39,49,60,69,82`) | `FormUtil.h` `switch(GetFormType())` + `static_cast`, FORM_ID codes | same, no RTTI | rows 6, 7. TESForm is the first base at offset 0 in all four classes, and the Load code above uses the same pointer as `this`. c1 reached the same conclusion | AGREE+SPOT |
| 9 | INGR YNAM | none | counts rows, no write | none | `src/MiscTypes.cpp:131-133` | PROVEN (no member or ID read) |

Not a row: `WriteAmmoShortName` etc. share the string-write path, rows 22 to 24 below.

### bodyParts

| # | Group | Member or ID | Fork/OG value | AE value | Evidence | Class |
|---|---|---|---|---|---|---|
| 10 | BPTD, RACE | form type codes BPTD 0x60, RACE 0x11; `As<BGSBodyPartData>` `BodyPartsChargen.cpp:202`, `As<TESRace>` `BodyPartsChargen.cpp:243` (no RTTI) | fork = CLAE | same | table ID 1309967 entries 96 and 17 (`formtypes.out`); BPTD Load checks `cmp eax,0x60` on the record type | AGREE+SPOT |
| 11 | BPTD BPTN | `BGSBodyPartData::partArray[26]` | fork 0x58 = CLAE 0x58, 26 entries | 0x58, bound 26 | BPTD Load (vslot 9, ID 2206212, 0x6173E0 / 0x617400 / 0x617720): `movzx eax,[rbx+0x115]; cmp al,0x1a; mov rdi,[r14+rsi*8+0x58]` (old part freed), then `mov [r14+rsi*8+0x58], rbx` at 0x617617 / 0x617637 / 0x617957 | AGREE+SPOT |
| 12 | BPTD BPTN | `BGSBodyPart::partName` | fork 0x40; CLAE has only `unk40` at 0x40 | 0x40 | BGSBodyPart::Load (ID 2206177, 0x614C90 / 0x614CB0 / 0x614FD0): first subrecord `cmp eax,0x4e545042` (BPTN), `lea rcx,[rsi+0x40]` + locstr at 0x614CC4 / 0x614CE4 / 0x615004. BPNN fills +0, BPNT fills +8 in the same function | AGREE+SPOT |
| 13 | BPTD | `BGSBodyPart` object size | fork 0x150; CLAE none | 0x150 | BPTD Load allocates `mov edx,0x150` (0x617578 / 0x617598 / 0x6178B8) and frees with 0x150. DSD does not allocate; informational | PROVEN (CLAE has no value) |
| 14 | BPTD | `BGSBodyPart.data.type` (PART_DATA 0x80 + 0x95) and flags 0x94 | fork 0x115 / 0x114; CLAE none | 0x115 / 0x114 | BGSBodyPart::Load stores local byte `[rbp-0x4b]` (default 0xFF from `mov [rbp-0x4c],0xff05ff00`) into `[rsi+0x115]` at 0x6150EA / 0x61510A / 0x61542A; BPTD Load indexes `partArray` with exactly this byte. DSD takes the index from the plugin file instead; the engine stores part n at `partArray[type]`, which is what `ApplyBodyParts` assumes | PROVEN (CLAE has no value) |
| 15 | RACE | `TESRace::faceRelatedData[2]` | fork 0x698 = CLAE 0x698 | 0x698 + sex*8 | RACE Load (vslot 9, ID 2208116, 0x686D90 / 0x686DB0 / 0x6870D0): `mov [r15+rsi*8+0x698], rax` with `rsi` = sex; helper IDs 2208172 / 2208173 / 2208174 index it the same way | AGREE+SPOT |
| 16 | RACE | sex index 0 = male, 1 = female; file order male block then female block | DSD assumption | holds | jump-table entry for `MNAM` sets `rsi = r8` (r8d zeroed at dispatch head 0x6872F0); entry for `FNAM` sets `esi = 1` (`mov r14d,1; mov esi,r14d`) at 0x6880F3 / 0x688113 / 0x688433 (MNAM at 0x68810A / 0x68812A / 0x68844A) | PROVEN (CLAE has no value) |
| 17 | RACE | `FaceRelatedData` object size and member order | fork 0x68 (offsets from declaration order) = CLAE | 0x68; tintingTemplate 0x00, morphGroups 0x38, facialBoneRegions 0x40, neckFatAdjustmentScales 0x50 | alloc `lea edx,[r9+0x68]` (0x688390 / 0x6883B0 / 0x688B33) + ctor 0x140691690; `movss [rax+0x50/0x54/0x58]` (neck scales); `[face]` tint, `[face+0x38]` morph groups (ID 2208172: `mov rax,[rax+0x38]`), `[face+0x40]` regions (ID 2208174: `mov rcx,[rcx+0x40]`) | AGREE+SPOT |
| 18 | RACE | `BSTArray` layout: data 0x00, capacity u32 0x08, size u32 0x10, size 0x18 | fork = CLAE | same | `Groups::AddGroup` (ID 2196536): `mov r8d,[r14+8]` (capacity), store into `[r14]` data, size at `[r14+0x10]`; ID 2208172 reads `[arr+0x10]` as count and `[arr]` as data, loops 8-byte elements. `new BSTArray` is 0x18 (`lea edx,[r9+0x18]`) | AGREE+SPOT |
| 19 | RACE TTGP | `Template::Groups` (size 0x18), `groups` at 0 | fork = CLAE | same | TTGP handler (0x688A9D / 0x68877D / 0x68875D cmp `TTGP`): `cmp qword [rax],0`, alloc 0x18, `mov [rax],rdi` | AGREE+SPOT |
| 20 | RACE TTGP | `Template::Group::name` 0, `entries` 0x10 (size 0x28) | fork = CLAE | same | `Groups::AddGroup` (ID 2196536, 0x3FFD80 / 0x3FFD90 / 0x4000B0): alloc 0x28, name set at +0, entries array ctor at `lea rcx,[rdi+0x10]`; `Group::Load` (ID 2196560, 0x400F80 / 0x400F90 / 0x4012B0): group name loaded with locstr on `this` (+0), entries pushed to `[this+0x10]` | AGREE+SPOT |
| 21 | RACE TTGP | `Template::Entry::name` 0x08 (after vptr) | fork = CLAE | 0x08 | Entry factory (ID 2196539, 0x3FFF70 / 0x3FFF80 / 0x4002A0): ctor `lea rcx,[rbx+8]` BGSLocalizedString, TESCondition at +0x10, slot +0x18, uniqueID +0x1C, flags +0x1E. `Group::Load`: later `TTGP` subrecords run locstr on `rdi+8` (0x401051 / 0x401061 / 0x401381 `lea rsi,[rdi+8]`). Matches DSD's "group, option, option, ..." order | AGREE+SPOT |
| 22 | RACE MPPN | `BGSCharacterMorph::Group::presets` 0x08, `Preset` stride 0x20, `Preset::name` 0 | fork = CLAE | same | MPPN handler (0x688944 / 0x688624 / 0x688604 cmp `MPPN`): `shl rcx,5; add rcx,[r12+8]; call locstr` (0x688A36 / 0x68871A / 0x6886F6). Group from ID 2208172 whose first member `[group]` is compared with the name | AGREE+SPOT |
| 23 | RACE MPPN | `morphGroups` is a pointer to `BSTArray<Group*>` | fork = CLAE | same | ID 2208172 (0x68CEE0 / 0x68CF00 / 0x68D220): `[this+sex*8+0x698] -> [+0x38] -> {data [0], count [+0x10]}`, element 8 bytes, compares `[element]` with the name (Group::name at +0) | AGREE+SPOT |
| 24 | RACE FMRN | `FacialBoneRegion::name` 0x28, `facialBoneRegions` pointer to `BSTArray<FacialBoneRegion*>` | fork = CLAE | 0x28 | FMRN handler (cmp `FMRN` at 0x688950 / 0x688630 / 0x688610): `mov rcx,[rbp+0xa8]; add rcx,0x28; call locstr` (current region from `FMRI`: ID 2208174 get or ID 2208173 create). ID 2208173 creates the region: `mov edx,0x90`, name ctor at `lea rcx,[rax+0x28]`, id at +0x88, array at `[face+0x40]` | AGREE+SPOT |

### Shared write and lookup path (both features)

| # | Group | Member or ID | Fork/OG value | AE value | Evidence | Class |
|---|---|---|---|---|---|---|
| 25 | all | `BGSLocalizedString` = one `BSFixedStringCS` (Entry*) at +0, size 8 | fork = CLAE | same | `Entry` layout from the engine: c_str getter ID 2268700 (`mov eax,[rdx+8]; shr eax,0xe; test al,1` shallow, else `lea rax,[rdx+0x18]`), length getter ID 2268706 (`mov eax,[rdx+0x10]`), `release` ID 2268720 (`and eax,0x3fff` refcount at +8, kShallow bit 14, `mov [rsi],0` at the end) | AGREE+SPOT |
| 26 | all | `"<ID=" prefix length 13` (`TextWrite::Holds`, `BGSLocalizedString::operator=`) | 13 | 13 | ID 2194229 (0x3510F0 / 0x351100 / 0x351420): compares the first 4 bytes with the literal `<ID=`, then `lea rax,[rbx+0xd]`; ID 2194231 does `add edi,-0xd` (`prefix.out`) | AGREE+SPOT |
| 27 | all | ID 2268729 `BSStringPool::GetEntry<char>` | fork 2nd slot = CLAE 2268729 | present in all six bins | 0x1678390 / 0x1678A60 / 0x16793E0 / 0x167D900 / 0x167DA00 / 0x167DD20; normalized equal in .191/.221/.240; Phase B shape proof | AGREE+SPOT |
| 28 | all | ID 2268720 `BSStringPool::Entry::release(Entry*&)` | fork 2nd slot = CLAE 2268720 | present in all six bins | 0x1677B10 / 0x16781E0 / 0x1678B60 / 0x167D080 / 0x167D180 / 0x167D4A0; body decrements the refcount with `lock cmpxchg [rcx+8]`, handles the shallow bit, clears the caller's pointer. Phase B only listed GetEntry; this one is new for these features | AGREE+SPOT |
| 29 | BPTD, RACE | `TESForm::sourceFiles` 0x08 -> `TESFileArray{data 0, size u32 8}`; ID 2193103 `TESForm::GetFile(int)` | fork 0x08; ID 2193103 (2nd slot) = CLAE 2193103 | same | present in all six bins (0x30F140 / 0x30F280 / 0x30F670 / 0x3120A0 / 0x3120B0 / 0x3123D0). Body (identical in .191/.221/.240): `mov r8,[rcx+8]`, size `[r8+8]`, index < 0 or >= size clamps to the last element, else `[data + idx*8]`. So `GetFile(0)` is the first (defining) file | AGREE+SPOT |
| 30 | BPTD, RACE | `TESFile::filename` 0x70 | fork = CLAE | 0x70 | Phase B (`ae-crosscheck.md`, `LookupModByName`) | AGREE+SPOT |
| 31 | both | form lookup by ID (`TESForm::GetFormByID` -> `GetAllForms`) | fork (2nd slot) 2689178 / 2689189; CLAE 4796465 / 4796476 | AE: engine function ID 2193092 (not a fork or CLAE wrapper) | fork IDs missing in 137/159/169/191/221/240 (checked); 2193092 present in all six (0x30E8F0 / 0x30EA30 / 0x30EE20 / 0x311850 / 0x311860 / 0x311B80); Phase B proof of the function | DIFFERS-PROVEN (fork 2689178 / 2689189 are absent in all six bins; CLAE 4796465 / 4796476 are present in all six bins but not used: DSD bypasses through ID 2193092, C1) |
| 32 | both (debug log only) | form-type name table ID 1309967 (`GetFormTypeString`), arguments are evaluated even when debug is off | OG ID | present in all six bins | 0x2EBFB60 / 0x2EC0B60 / 0x2EC5B60 / 0x2ED0B60 / 0x2ED0B60 / 0x2EDBB60; entries read | AGREE+SPOT |

### Scope limits (not part of the RE-member / ID proof)

- **Versions without exes.** Offsets are proven on .191/.221/.240 (the proof set of the evidence standard). The IDs are
  proven present in all six bins; no exe exists for .137/.159/.169 here, so a layout change there would not show.
  Same limit as Phase B.
- **BPND byte 0x65.** `ReadBodyPartTypes` indexes by the plugin-file BPND type byte. That is a plugin file-format fact
  (checked on OG against Fallout4.esm), not an RE struct member or a REL::ID, so it is outside this proof. Rows 11 and 14
  prove the runtime half on AE: `partArray[type]`, with `type` from a local struct that `BGSBodyPart::Load` fills from
  the file through helper 0x616F40. BPTD is READY on that basis; if the file offset were ever wrong, OG would be wrong too.

## Verdict per group

| Group | Verdict | What the AE port needs besides the proof |
|---|---|---|
| AMMO ONAM (`TESAmmo::shortDesc`) | READY | swap `GetFormByID` (C1) |
| ALCH DNAM (`AlchemyItem::data.addictionName`) | READY | swap `GetFormByID` (C1) |
| DOOR ONAM / CNAM | READY | swap `GetFormByID` (C1) |
| AVIF ANAM | READY **only with a per-runtime offset** (AE 0x1A0, OG 0x160) | C1 and C2. Left at 0x160 it writes into the wrong member on AE |
| INGR YNAM | READY (no memory access, only counts rows) | none |
| BPTD BPTN | READY | C1 |
| RACE TTGP | READY | C1 |
| RACE MPPN | READY | C1 |
| RACE FMRN | READY | C1 |
| Shared write path (BGSLocalizedString assign, GetFile, filename) | READY | none |

Required code changes (not done here, listed for the implementer):

- **C1** `src/MiscTypes.cpp:119` and `src/BodyPartsChargen.cpp:54,197,242` call `RE::TESForm::GetFormByID`. On AE that
  wrapper reads the form map and lock through fork IDs 2689178 / 2689189. Both are absent in all six 1.11.x bins (checked
  below). Use `DSD::LookupFormByID` (`src/Runtime.h`, engine function ID 2193092, Phase B proof) in these four places.
- **C2** `WriteAbbreviation` (`src/MiscTypes.cpp:76-87`) hard-codes `kOffsetOG = 0x160`. On AE use 0x1A0
  (`actorValue->abbreviation` of the fork header is already 0x1A0).
- **C3** Set `miscTypes` and `bodyParts` in `MakeFeatures(Runtime::AE)`.

Nothing else in either file touches game memory. `As<>` is not RTTI (see below), so c1's RTTI work is not a dependency.

## `As<>` call sites (RTTI dependency list)

All seven are form-type switches (`FormUtil.h`), not RTTI; each needs only the form type byte and the code in rows 6 and 10.

| Call site | From -> To | Code |
|---|---|---|
| `src/MiscTypes.cpp:39` | TESForm* -> TESAmmo* | 0x2C |
| `src/MiscTypes.cpp:49` | TESForm* -> AlchemyItem* | 0x30 |
| `src/MiscTypes.cpp:60` | TESForm* -> TESObjectDOOR* | 0x20 |
| `src/MiscTypes.cpp:69` | TESForm* -> TESObjectDOOR* | 0x20 |
| `src/MiscTypes.cpp:82` | TESForm* -> ActorValueInfo* | 0x62 |
| `src/BodyPartsChargen.cpp:202` | TESForm* -> BGSBodyPartData* | 0x60 |
| `src/BodyPartsChargen.cpp:243` | TESForm* -> TESRace* | 0x11 |

## Details worth knowing

- **RACE load order of events.** `TTGP` at RACE level starts a new group (`Groups::AddGroup(this, name=null)`, then
  `Group::Load`). Inside `Group::Load` the first `TTGP` names the group, `TETI` creates an entry, and each following
  `TTGP` names that entry, until `TTGE`. DSD's "group, option, option, ..., next group" order matches.
- **Why `As<>` needs no RTTI here.** Six types, all in `FormUtil.h`; codes in row 6 and 10, all confirmed.
- **CLAE ActorValueInfo.** Do not use CommonLibF4AE's `ActorValueInfo.h` for AE offsets. It still has the OG layout.
  Spot checks on the other headers all passed, but this single failure means CLAE headers are not an authority by
  themselves; every row above was taken from the exes.
- **Guard still needed.** `ApplyRace` builds slot lists (reads the arrays) before the count check. With rows 15 to 24
  proven this is safe on .191/.221/.240; on an unknown 1.11.x version the existing count guard only prevents a wrong
  write, not a bad read (same remark as `runtime-inventory.md` 4.8).
- **Docs to correct (not edited).** `docs/runtime-support.md:13` ("member offsets other than AVIF ANAM unproven") and
  `docs/runtime-support.md:144` ("bodyParts chain unproven") become outdated once C1 to C3 land.
