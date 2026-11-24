STATUS: DONE

# Phase C1: type checks and casts on AE (RTTI)

Scope: `ConstTranslation.cpp`, `PortedConstFields.cpp`, `TemplateNames.cpp`, `MiscTypes.cpp`, `BodyPartsChargen.cpp`,
`RuntimeText.cpp`, `NameRefresh.cpp`, `DialogueOptions.cpp`, `Dialogue.cpp`. Analysis only; no source changed.
Evidence scripts and caches: `phaseC/c1/` (`rtti.py` RTTI parser, `an2.py` layout compare, `ctor.py`/`ctor_all.py`
constructor check, `ids.py` CommonLibF4AE ID check, `off2.cpp` fork offset probe). Exes: .191, .221, .240 (AE) and OG
(`.rdata` readable, used only as a layout reference).

## Main finding

`RE::TESForm::As<T>()` in the fork is **not RTTI**. `extern/CommonLibF4/.../Bethesda/FormUtil.h:43-` is
`switch (GetFormType())` over `T::FORM_ID`, then a compile-time `static_cast<const Concrete*>(this)`.
So every `As<>` call site needs only two facts on AE: (1) the form type byte (+0x1A) holds the fork's `ENUM_FORM_ID` code
for that class, (2) the static base offsets of the fork's class layout (TESForm at 0, TESFullName/TESDescription at fixed
offsets) are the same in the AE class. Both are proven below. **No `As<>` replacement is needed**; the "RTTI TypeDescriptor
IDs absent on AE" problem in `docs/ae-crosscheck.md` row "CopyComponent" affects only one call: `fallout_cast` at
`ConstTranslation.cpp:130`. That one has a cheaper replacement than any RTTI lookup (vtable identity, section 3).

`docs/runtime-support.md:148` and `docs/ae-crosscheck.md:71` say `As<>` needs RTTI. That is wrong and should be corrected
by whoever owns those files.

## Deliverable table

Layout key: `FULL`/`DESC`/`FORM` = offset of TESFullName / TESDescription / TESForm inside the class, `-` = class has no
such base. "Same" means fork (compiled with MSVC from the fork headers, `off2.cpp`) = OG RTTI = AE .191 = .221 = .240.

| Use (file:line) | From -> To | AE replacement | Evidence | Class |
|---|---|---|---|---|
| ConstTranslation.cpp:45 | TESForm* -> TESFullName* (`As`, all FULL types) | none: keep `As<TESFullName>`. Static base offset, e.g. NPC_ 288, QUST 40, ARMO/WEAP/MISC/ALCH/AMMO/DOOR/TERM 104, CONT 128, FACT/RACE/PERK/AVIF/MESG/CELL/WRLD/DIAL 32. Classes with `-` (e.g. LVLI, REGN, REFR, LSCR, BPTD) keep returning null and fall to the sparse-map path, as on OG | Layout section 1: 144 classes compared, FULL equal in fork, OG, .191/.221/.240 for all 144 | PROVEN (AGREE+SPOT) |
| ConstTranslation.cpp:130 | BaseFormComponent* -> TESForm* (`fallout_cast`, runtime RTTI) | **DIFFERS.** Fork `RTDynamicCast` uses OG ID 84112 (missing in every 1.11.x bin) and RTTI IDs 2764465/2764237 (missing in all six bins). Replace on AE with a vtable identity test: `*(void**)a_from == VTABLE::TESNPC[5]` (ID 1206734) then `form = (TESForm*)((char*)a_from - 288)`. Resolve the vtable address once at install; per call = one load + compare, no RTTI, no ID lookup, no engine call | Section 3. ID 1206734 is exact in all six bins, RVA 0x24CE808 / 0x24CE818 / 0x24D6878 (.191/.221/.240) = TESNPC vtable whose COL offset is 288 (`rtti.py`). TESNPC FULL offset 288 same everywhere. No class derives from TESNPC in 191/221/240, so the identity test equals "is a TESNPC". OG keeps `fallout_cast` | DIFFERS (fork 84112 + RTTI IDs absent; AE value = vtable ID 1206734, fork has the same ID in `VTABLE::TESNPC[5]`) |
| ConstTranslation.cpp:130 (alt) | same, if a real cast is ever wanted | `__RTDynamicCast` ID 4818455 (import thunk, `VCRUNTIME140!__RTDynamicCast`) with TD IDs BaseFormComponent 4839768, TESForm 4839494 (from CommonLibF4AE) | IDs exact in all six bins; TD name at RVA+0x10 = `.?AVBaseFormComponent@@` / `.?AVTESForm@@` in .191/.221/.240 (`ids.py`); thunk bytes `ff 25` at 0x22BCB9E/0x22BCC7E/0x22C46AE, IAT name checked | PROVEN (option, not needed) |
| ConstTranslation.cpp:161 | TESForm* -> TESQuest* | none | QUST code 0x50; TESQuest ctor stores `byte [this+0x1A] = 0x50` in .191/.221/.240; FORM 0 everywhere | PROVEN (AGREE+SPOT) |
| ConstTranslation.cpp:177, :195 | TESForm* -> BGSMessage* | none | MESG code 0x6C; ctor store 0x6C in all three; FORM 0 | PROVEN (AGREE+SPOT) |
| ConstTranslation.cpp:207, :220, :235, :277, :286 | TESForm* -> BGSTerminal* | none | TERM code 0x37; ctor store 0x37 in all three; FORM 0 | PROVEN (AGREE+SPOT) |
| ConstTranslation.cpp:258 | `PointsToForm`: first qword of a union is inside `.rdata` | none; `REL::Module::segment(rdata)` looks sections up by name (`Module.cpp:66-86`), so the extra `.interpr` section does not shift it | All 144 primary form vtables lie in `.rdata` of .191 [0x2438000,0x2ECCB42), .221 [0x2438000,0x2ECCBE2), .240 [0x2440000,0x2ED7362) | PROVEN |
| ConstTranslation.cpp:427-437 | TESFullName vtable slot 6 vs hook target | **DIFFERS in ID only.** `RE::VTABLE::TESFullName[0]` (487988) is valid on AE. The OG target ID 1510422 is absent in all six 1.11.x bins (`id2rva` = None). AE target = ID 2193213 | VTABLE 487988 = RVA 0x24625E0/0x24625E0/0x246A640 = RTTI vtable of TESFullName; slot 6 = 0x315B90/0x315BA0/0x315EC0 = RVA of ID 2193213 in .191/.221/.240; ID 2193213 present in all six bins (0x312C30, 0x312D70, 0x313160, 0x315B90, 0x315BA0, 0x315EC0). TESNPC's own TESFullName vtable slot 6 is the same function (no override) | DIFFERS (ID 1510422 -> 2193213) |
| PortedConstFields.cpp:25 | TESForm* -> TESLoadScreen* | none | LSCR code 0x54; ctor store 0x54 in all three; FORM 0 | PROVEN (AGREE+SPOT) |
| PortedConstFields.cpp:38 | TESForm* -> EffectSetting* | none | MGEF 0x15; ctor store 0x15; FORM 0 (FULL 32 same) | PROVEN (AGREE+SPOT) |
| PortedConstFields.cpp:51 | TESForm* -> TESNPC* | none | NPC_ 0x2D; ctor store 0x2D; FORM 0 | PROVEN (AGREE+SPOT) |
| PortedConstFields.cpp:79 | TESForm* -> TESObjectREFR* | none. `As` matches formType REFR (0x40) only, not ACHR 0x41, same as OG | REFR 0x40; ctor store 0x40; FORM 0 | PROVEN (AGREE+SPOT) |
| PortedConstFields.cpp:112 | TESForm* -> TESRegion* | none | REGN 0x3D; ctor store 0x3D; FORM 0 | PROVEN (AGREE+SPOT) |
| PortedConstFields.cpp:147 | TESForm* -> BGSPerk* | none | PERK 0x5F; ctor store 0x5F; FORM 0 | PROVEN (AGREE+SPOT) |
| PortedConstFields.cpp:152-153 | vptr identity: BGSEntryPointPerkEntry / ...ActivateChoice vtables | none: fork `VTABLE::...[0]` IDs 846981 / 859152 are valid on AE | IDs exact in all six bins; RVAs 0x24CA3D8/0x24CA3E8/0x24D2448 and 0x24C8688/0x24C8698/0x24D06F8 = RTTI vtable (COL offset 0) of each class in .191/.221/.240 | PROVEN (AGREE+SPOT) |
| TemplateNames.cpp:47 | TESForm* -> TESFullName* (ARMO, WEAP, NPC_ only) | none | FULL 104 (ARMO, WEAP), 288 (NPC_) same everywhere; ARMO 0x1D, WEAP 0x2B ctor stores in all three | PROVEN (AGREE+SPOT) |
| TemplateNames.cpp:52 | `static_cast<TESBoundObject*>(TESForm*)` | none (offset 0). Member `objectTemplate` at +0x30 is not an RTTI matter (covered by another phase C layout proof) | TESBoundObject: TESForm base at mdisp 0 in .191/.221/.240/OG | PROVEN (cast only) |
| MiscTypes.cpp:39 | TESForm* -> TESAmmo* | none | AMMO 0x2C; ctor store 0x2C; FORM 0 | PROVEN (AGREE+SPOT) |
| MiscTypes.cpp:49 | TESForm* -> AlchemyItem* | none | ALCH 0x30; ctor store 0x30; FORM 0 | PROVEN (AGREE+SPOT) |
| MiscTypes.cpp:60, :69 | TESForm* -> TESObjectDOOR* | none | DOOR 0x20; ctor store 0x20; FORM 0 | PROVEN (AGREE+SPOT) |
| MiscTypes.cpp:82 | TESForm* -> ActorValueInfo* | none | AVIF 0x62; ctor store 0x62; FORM 0 | PROVEN (AGREE+SPOT) |
| BodyPartsChargen.cpp:202 | TESForm* -> BGSBodyPartData* | none | BPTD 0x60; ctor store 0x60; FORM 0 | PROVEN (AGREE+SPOT) |
| BodyPartsChargen.cpp:243 | TESForm* -> TESRace* | none | RACE 0x11; ctor store 0x11; FORM 0 | PROVEN (AGREE+SPOT) |
| RuntimeText.cpp:43 | TESForm* -> TESDescription* (types with DESC) | none: `As<TESDescription>` static offsets: ARMO 552, WEAP 368, AMMO 280, ALCH 400, BOOK 232, SPEL/SCRL 240, CLAS/RACE/PERK/AVIF/MESG 48, same in fork/OG/.191/.221/.240; classes with `-` return null as on OG | Layout section 1 (DESC column, 144 classes) | PROVEN (AGREE+SPOT) |
| NameRefresh.cpp:84, :291 | TESBoundObject* -> TESFullName* (`As` is a TESForm member) | none. TESBoundObject/TESObject carry TESForm at 0, so the `this` passed to `As` equals the form pointer | TESObject and TESBoundObject: TESForm mdisp 0 in all four images; FULL table as row 1 | PROVEN (AGREE+SPOT) |
| DialogueOptions.cpp:350 | `static_cast<BGSSceneActionPlayerDialogue*>(BGSSceneAction*)` | none (offset 0; the type is decided by `GetActionType()`, not RTTI) | BGSSceneActionPlayerDialogue: BGSSceneAction base at mdisp 0 in all four images | PROVEN (cast only) |
| Dialogue.cpp (all) | no `As`, `fallout_cast`, vtable or RTTI use (only `reinterpret_cast` of code bytes) | n/a | grep of the file | n/a (no RTTI use) |

No `skyrim_cast`, `netimmerse_cast` or `dynamic_cast` appears in the nine files (grep `As<|fallout_cast|skyrim_cast|netimmerse|dynamic_cast|VTABLE|RTTI`).

Counts (table rows): 24 PROVEN (one of them is the optional real-cast alternative), 2 DIFFERS, 0 UNPROVEN, 1 n/a (Dialogue.cpp has no RTTI use).

## 1. Layout and form type proof (for every `As<>`)

### 1.1 Form type codes
- AE form type name table, Address Library ID 1309967: RVA 0x2ED0B60 (.191, .221), 0x2EDBB60 (.240). 159 entries of stride 0x18
  `{code, ?, "NAME"}`. The three tables are byte-identical in `{code, name}` and **all 159 entries equal the fork's
  `ENUM_FORM_ID`** (`kNAME` index = code), including every code DSD uses (GMST 3, KYWD 4, FACT 14, RACE 17, MGEF 21,
  ACTI 27, ARMO 29, BOOK 30, CONT 31, DOOR 32, MISC 35, FLOR 41, FURN 42, WEAP 43, AMMO 44, NPC_ 45, ALCH 48, TERM 55,
  LVLI 56, REGN 61, REFR 64, QUST 80, LSCR 84, PERK 95, BPTD 96, AVIF 98, MESG 108).
- That table only maps signature to code. The class-to-code binding comes from the constructors: for each concrete class the
  function that stores the class vtable (found by xref of the COL offset-0 vtable) also does `mov byte ptr [reg+0x1A], imm8`
  with the fork's `FORM_ID`. 136 of 144 `As<>` classes confirmed in .191, .221 and .240 (`ctor_all.py`, 408/432 checks).
  The 8 not found by this method (TESForm, BGSKeyword, BGSLocationRefType, BGSAction, BGSTransform, BGSComponent,
  BGSTextureSet, Actor) take the code from a base-constructor parameter, and none is an `As<>` target in DSD
  (their codes are still in the name table).
- Form type byte +0x1A: already PROVEN earlier (`docs/ae-crosscheck.md` line 52).

### 1.2 Static base offsets
`rtti.py` parses all COL / ClassHierarchyDescriptor / BaseClassArray data. For the complete-object COL (offset 0) of each of
the 144 fork `As<>` classes (the full `FormUtil.h` list minus `BGSMod::Attachment::Mod`, which DSD does not use), the PMD
`mdisp` of the TESFullName, TESDescription and TESForm base was compared with:
- the fork's compile-time offset (`off2.cpp`: MSVC, `static_cast<Base*>` on a dummy object, from the fork headers);
- the OG exe RTTI.

Result: **0 mismatches** (`an2.py`): for all 144 classes FULL, DESC and FORM are equal in fork, OG, .191, .221, .240.
FORM is 0 for all. Example (fork = OG = .191 = .221 = .240): TESNPC FULL 288; TESQuest FULL 40; TESObjectARMO FULL 104 DESC 552;
TESObjectWEAP FULL 104 DESC 368; TESAmmo FULL 104 DESC 280; AlchemyItem FULL 104 DESC 400; BGSMessage FULL 32 DESC 48;
TESRace FULL 32 DESC 48; ActorValueInfo FULL 32 DESC 48; BGSTerminal FULL 104; TESLevItem / TESRegion / TESLoadScreen /
TESObjectREFR / BGSBodyPartData: no TESFullName (fork and AE both).
The AVIF ANAM member offset (+0x160 OG, +0x1A0 AE) is a member offset, not a base offset; it is outside this scope.

Caveat (honest limit): this proves the base placement, not the layout of members inside those components (for example
`TESFullName::fullName` at +8, which is proven elsewhere).

## 2. CommonLibF4AE comparison (method change)

| Item | Fork (2nd slot / own value) | CommonLibF4AE | Result |
|---|---|---|---|
| RTTI TypeDescriptor IDs, 151 names (all 146 `As<>` classes less one nested class, plus BaseFormComponent, TESForm, TESFullName, TESDescription, BGSKeywordForm, TESObject, TESBoundObject) | `RelocationID(og, 27642xx)`: the NG ID is absent from all six 1.11.x bins (checked TESForm 2764237, TESFullName 2764489, BaseFormComponent 2764465) | e.g. TESFullName 4839792, TESForm 4839494, BaseFormComponent 4839768, TESNPC 4840540 | CLAE-ONLY, **verified for all 151** (not 1 in 5): ID exact in all six bins, and the bytes at RVA+0x10 are the expected `.?AV<name>@@` in .191, .221 and .240. 151/151 pass. Class: AGREE+SPOT (full verification) |
| VTABLE IDs used by DSD: TESFullName[0] 487988, TESLevItem[1] 1381610, BGSEntryPointPerkEntry[0] 846981, ...ActivateChoice[0] 859152, TESNPC[5] 1206734 | same IDs | same IDs | AGREE+SPOT: all five exact in six bins and equal to the RTTI-derived vtable RVA in .191/.221/.240 (5/5). Whole-file note: 7564 VTABLE names are in both headers, 7229 identical (335 differ; not in DSD scope, not examined) |
| `__RTDynamicCast` | 84112 (OG function, missing in 1.11.x) | 4818455 | DIFFERS-PROVEN: AE exe decides, 4818455 (import thunk, IAT name `VCRUNTIME140!__RTDynamicCast`) |
| `ENUM_FORM_ID` list, base offsets of classes | fork headers | not re-diffed; decided from the AE exes (sections 1.1, 1.2) | n/a |

Spot checks passed 5/5 + 151/151, failed 0. CommonLibF4AE is trustworthy for its RTTI and vtable IDs in this scope.

## 3. Replacement for `fallout_cast` in the NPC FULL copy hook (the only real RTTI use)

`FullNameCopyComponent::thunk(TESFullName* a_this, BaseFormComponent* a_from)` wants the source TESNPC form. `a_from`
is the TESFullName sub-object of the source object (TESFullName derives BaseFormComponent at offset 0). The hook needs an
answer only for a TESNPC. A TESNPC's TESFullName sub-object holds the secondary vtable of class TESNPC with COL offset 288, so
the vptr identifies the complete type exactly:

```cpp
// resolved once, in InstallHooks (AE branch)
static const std::uintptr_t npcFullNameVtable =
    REL::Relocation<std::uintptr_t>{ RE::VTABLE::TESNPC[5] }.address();   // ID 1206734, RVA 0x24CE808/0x24CE818/0x24D6878
constexpr std::ptrdiff_t kNpcFullNameOffset = 288;                          // = offsetof FULL in TESNPC, proven same on OG and AE

// in the thunk, AE branch (OG keeps RE::fallout_cast)
RE::TESForm* form = nullptr;
if (a_from && *reinterpret_cast<const std::uintptr_t*>(a_from) == npcFullNameVtable) {
    form = reinterpret_cast<RE::TESForm*>(reinterpret_cast<std::byte*>(a_from) - kNpcFullNameOffset);
}
if (form && form->GetFormType() == RE::ENUM_FORM_ID::kNPC_ && !IsPlayerForm(form)) { ... }
```

Safety: the vptr read is at `a_from` itself, never below it, so no unmapped read. A source that is not a TESNPC (any other
form or a loose TESFullName) never has this vptr, so `form` stays null, which is what the dynamic cast followed by the type
check gives on OG. No class derives from TESNPC in .191/.221/.240 (scan of all BaseClassArrays), so there is no derived-class loss.
Cost per call: one load, one compare. It replaces the engine's `__RTDynamicCast` walk. The AE install also needs the target
ID mapped to 2193213 (row `:427-437`).

Residual risk: a vptr is only read as the identity of the secondary vtable; if a future 1.11.x re-orders the 19 TESNPC vtables
the ID 1206734 would be stale. The load-time guard is cheap and should be kept: at install, check that the COL at
`vtable-8` has offset 288 and TypeDescriptor name `.?AVTESNPC@@` (same pattern as the existing `check_vtable.py`).

## 4. `DSD::As<T>` mechanism proposal

Because `As<>` is not RTTI, **no new `DSD::As<T>` is needed**. The fork's `As<>` is usable on AE as is (zero runtime cost).
If a general engine cast is wanted later (not required by any file in this scope), these are the options:

### Option A: per-type AE ID table (recommended for a general cast)
```cpp
// Address Library IDs of the RTTI TypeDescriptors (CommonLibF4AE, verified in 6 bins and the 3 exes)
inline constexpr std::uint64_t kAeRttiBaseFormComponent = 4839768;
inline constexpr std::uint64_t kAeRttiTESForm           = 4839494;
inline constexpr std::uint64_t kAeRtDynamicCast         = 4818455;   // import thunk
template <class To> To* AeCast(void* p, std::uint64_t fromID);        // resolved once: REL::Relocation<void*>{ REL::ID(id) }
```
Cost: zero scan, one `REL::ID` resolve per type at load. Risk: a future 1.11.x bin that lacks the ID fails loudly (patch
`commonlibf4-runtime-detection.patch` already makes a missing ID fatal), which is the safe failure.

### Option B: scan the module for `.?AV<name>@@`
Search the initialised part of `.data` (`SizeOfRawData`, not `VirtualSize`) for the name bytes; TypeDescriptor = name - 0x10.
Measured offline on .240 (`.data` raw 2,151,424 bytes, names at file offsets about 0.80 to 0.83 MB):
- Python `bytes.find`, 5 names: **1.02 ms total** (about 10 GB/s).
- Whole file (55 MB), 1 name with `re`: 28.7 ms.
- C++ estimate, `memmem` or `std::string_view::find` over 2.15 MB: about 0.2 to 1 ms per name; with a naive search 2 to 4 ms per
  name. Do not scan `VirtualSize` (16.4 MB, mostly untouched bss): it would commit about 14 MB of zero pages.
Survives new 1.11.x builds without ID work, but needs a PE-section parse and a validity check (type_info vptr in `.rdata`).

Recommendation: **use neither for this scope.** Use the vtable identity test (section 3) for the one RTTI site; keep
Option A in reserve (its IDs are already verified) and Option B only if a future update breaks the IDs.

## 5. Notes and open flags

- Correct `docs/runtime-support.md:148` and `docs/ae-crosscheck.md:71`: `As<>` is not RTTI; the blocking item is the single
  `fallout_cast` plus the OG-only hook target ID 1510422 (AE 2193213).
- `ConstTranslation::InstallHooks` passes OG ID 1510422 to `REL::ID`; on AE that ID is absent (loud failure after the patch). The
  AE path needs ID 2193213 and the AE hook-byte guard (`AeHookBytes.h` is outside this document's scope).
- `GetSparseFullNameMap()` (used when `As<TESFullName>` is null) is not RTTI and is not covered here; its ID and layout belong to
  another phase C proof.
- The offset probe `off2.cpp` links with unresolved symbols stubbed; it crashes at exit after printing (harmless, output was
  written line by line and compared 144/144).
