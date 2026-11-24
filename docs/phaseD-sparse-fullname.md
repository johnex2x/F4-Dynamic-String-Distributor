STATUS: DONE (analysis proven in .191/.221/.240; in-game result UNVERIFIED)

# Phase D2: sparse FULL name map on AE

Goal: on AE, `SetName` (src/ConstTranslation.cpp) returned "wrong type" for every form without a TESFullName component. A log on AE 1.11.240: KYWD FULL 412, STAT 2797, SCOL 362, FLST 349, MSTT 235, LIGH 54. OG writes them through the sparse map. This phase proves the map on AE and turns the path on.

Scratch evidence: `phaseD/sparse/` (`a1`..`a16.py`, `check_guard.py`, `idaudit.py`). All RVAs below are .240 unless a triple is given (.191 / .221 / .240).

## Result

| Form type | AE | Evidence |
|---|---|---|
| KYWD (BGSKeyword; also BGSAction, BGSLocationRefType share the function) | ON | slot 9 (subrecord load, ID 2206417): `FULL` branch and `NNAM` branch both end in the map setter (ID 2193219) |
| STAT (TESObjectSTAT) | ON | slot 9 (ID 2198825): `FULL` branch -> setter |
| SCOL (BGSStaticCollection) | ON | slot 9 (ID 2197655): `FULL` branch -> setter |
| FLST (BGSListForm) | ON | slot 9 (ID 2203253): `FULL` branch -> setter |
| MSTT (BGSMovableStatic) | ON | slot 9 (ID 2197483): `FULL` branch (binary-search `je`) -> setter |
| LIGH (TESObjectLIGH) | ON | slot 9 (ID 2198249): `FULL` branch -> setter |

ON means: the load code of that type stores its FULL in this map, so DSD's write reaches the text the game reads. A form that has no entry in the map (empty FULL in the plugin) still counts as "wrong type", as on OG. The whole group turns OFF (logged once) only when a run-time guard fails.

Route: **direct map**, not the engine lookup. Reason: the engine lookup (ID 2193216) returns the string length, and the sibling (ID 2193217) returns `const char*` (a pointer into a temporary copy), so neither returns the stored slot to write through. The setter (ID 2193219, `SetSparseFullName(form, BGSLocalizedString*)`) erases when the text is empty and inserts otherwise: calling it would add entries for forms that had none (OG does not) and needs a constructed BGSLocalizedString. The fork's `BSTHashMap::find` over the proven object reproduces the OG behaviour with no engine call.

## Proof

### The map object (ID 2661402)

ID 2661402 is an `.data` address in all six bins (.240 RVA 0x2EDCB00; .191/.221 0x2ED1B00). The engine functions that touch the map take this address as "this" (static init at 0x3161F0 passes `lea rcx, [ID]`; the insert function 0x316450 uses `[rdi+4]` capacity, `[rdi+8]` free, `[rdi+0xC]` good, `[rdi+0x10]` sentinel, `[rdi+0x20]` entries, with rdi = ID address). The fork's object starts 8 bytes earlier (`REL::RelocationID(226372, 2661402), -0x8`: leading `_pad00`), which puts the same fields at +0xC, +0x10, +0x14, +0x18, +0x28. Both views name the same bytes. CommonLibF4AE has the same ID and the same -0x8 (FormComponents.h:1428-1433). DSD uses the object address `ID - 8`.

Three engine functions read the map (xrefs to the object, identical set in .191/.221/.240): ID 2193216 (length), ID 2193217 (c_str), ID 2193219 (set/erase), plus the insert 0x316450 (called only from 2193219) and the static initialiser. 

### Layout and hash (the lookup, ID 2193216; .191/.221/.240 = 0x315CD0 / 0x315CE0 / 0x316000)

The lookup first `__RTDynamicCast`s the form to TESFullName and, for forms with the component, tail-calls vtable slot 7. Otherwise (.240):

```
31605B  mov rdx, [rip+..]      ; entries   = object+0x28   (0x2EDCB20)
316067  mov ecx, [rip+..]      ; capacity  = object+0x0C   (0x2EDCB04)
31606D  mov eax, [rsp+0x48]    ; CRC32 of the form pointer (call 0x1659300, init 0)
316071  dec ecx / and rcx,rax  ; index = hash & (capacity-1)
316076  lea rax,[rcx+rcx*2] / lea rdx,[rdx+rax*8]   ; entry = entries + index*0x18
316083  cmp [rdx+0x10], rdi    ; next == 0  -> empty slot
316089  mov rax, [rip+..]      ; sentinel  = object+0x18   (0x2EDCB10)
316090  cmp [rdx], rbx         ; key == form
316095  mov rdx,[rdx+0x10]     ; follow next until next == sentinel
3160AA  add rdx, 8             ; value = BGSLocalizedString at entry+8
```

`check_guard.py` decodes the three RIP-relative loads in all three exes: they resolve to object+0x28, +0xC, +0x18 exactly (.191/.221: 0x2ED1B20/04/10; .240: 0x2EDCB20/04/10). The sentinel value stored there is an address inside `.data` (object-8).

Hash: ID 2267895 (0x1658EE0 / 0x1658FE0 / 0x1659300, 222 bytes in all three). It runs 8 table lookups (the 8 bytes of the key), starts from `r8d = 0`, ends with `mov [rcx], eax` and has no final inversion. Its 256-entry table (`rip+0x102207c`) equals the standard CRC-32 table in all three exes. That is `RE::BSCRC32<const TESForm*>` in the fork (`detail::GenerateCRC32` over the pointer's 8 bytes, init 0; CRC.h is identical in the fork and CommonLibF4AE). Entry stride 0x18 with key +0, BGSLocalizedString +8, next +0x10 matches the fork's `entry_type` (value_type = `BSTTuple<const Key, T>`, `next` last, empty = `next == nullptr`). BSTHashMap.h differs between the fork and CommonLibF4AE only in the iterator implementation, not in layout.

### Value type

Entry+8 is handled with the same two helpers the engine uses for `TESFullName::fullName` (+8 of the component): copy-assign 0x14167CD70 (in `CopyComponent`, ID 2193213: `lea rdx,[rax+8]` -> `lea rcx,[rsi+8]`; in 2193217: `add rdx,8` then the same call and the c_str reader 0x140351420), release 0x14167CD00 (erase path 2193219: `lea rcx,[rbx+8]`) and copy-construct 0x14167CC90 (insert 0x316450: `lea rcx,[r14+8]`). So the value is a BGSLocalizedString, the type `TextWrite::Assign` already writes on AE (phase C3). The setter's emptiness test (0x351620) compares the string length and a 4-byte prefix with a 13-byte length (the size of a `<ID=xxxxxxxx>` placeholder), the same placeholder form the component's string uses; this is read from the code, not proven by a run.

### Which forms store FULL here (vtable slot 9, subrecord load)

The six functions are in the vtables of the class named below (RTTI col at vtable-8). Each contains `cmp eax, 'FULL'` (0x4C4C5546); its branch begins with BGSLocalizedString ctor (ID 2194227), `BGSLocalizedString::Load` (ID 2194235), then the setter (ID 2193219). For KYWD the `NNAM` (0x4D414E4E) branch also reaches the setter (ctor ID 2194228 from a char buffer). Same in .191/.221/.240 (`a13.py`, `a15.py`, `a16.py`):

| Class | Function ID | .191 / .221 / .240 start |
|---|---|---|
| BGSMovableStatic | 2197483 | 0x432150 / 0x432160 / 0x432480 |
| BGSStaticCollection | 2197655 | 0x43AA70 / 0x43AA80 / 0x43ADA0 |
| TESObjectLIGH | 2198249 | 0x4539B0 / 0x4539C0 / 0x453CE0 |
| TESObjectSTAT | 2198825 | 0x471530 / 0x471540 / 0x471860 |
| BGSListForm | 2203253 | 0x588AD0 / 0x588AE0 / 0x588E00 |
| BGSKeyword (+ BGSAction, BGSLocationRefType) | 2206417 | 0x61E1A0 / 0x61E1C0 / 0x61E4E0 |

One more caller, ID 2193074, is a global string setter (not a slot-9 load); it is not used.

## Implementation

`src/ConstTranslation.cpp`: `FindSparseNameMapAE()` (once, cached by `GetSparseNameMap()`), `SetName` uses `GetSparseNameMap()` for both runtimes (OG: the fork's `GetSparseFullNameMap()`, unchanged behaviour). Guard order (any failure: log once, map OFF, the rest of constTranslation unaffected):

1. `HookUtil::CodeMatches` of the lookup (158 bytes, `AeHookBytes::kSparseNameLookup`) and the CRC helper (61 bytes, `kSparseNameHash`); generated by `ae/gen_hookbytes.py` from .191/.221/.240.
2. The three RIP-relative loads of the lookup must resolve to object+0x28, +0xC, +0x18.
3. Object inside `.data`; capacity zero or a power of two; free <= capacity; good < capacity; sentinel inside `.data`; entries non-null and aligned when capacity > 0.
4. Live check over the filled map (one pass over capacity slots, no allocation): used slots + free == capacity, and at least 90% of entries sit in the slot their CRC hashes to or in a chain that starts there. A wrong hash or entry size fails this.

Cost: nothing per call beyond OG (`find` on a static pointer); the guard runs once at the first apply. The log line "FULL of forms without a TESFullName component ... is not translated on this runtime" now appears only when the guard fails; otherwise one `Sparse FULL map ...: capacity N, M entries` line.

## ID audit (exact members of all six 1.11.x bins: 137, 159, 169, 191, 221, 240)

| ID | Use | Six bins |
|---|---|---|
| 2661402 | map object address (+8), read and written through | yes |
| 2193216 | lookup code, bytes checked, never called | yes |
| 2267895 | CRC helper, bytes checked, never called | yes |
| 2193217, 2193219, 2194227, 2194228, 2194235, 2192538, 2193213 | cited as evidence only | yes (2194235, 2193217, 2193219 re-checked) |

## Counts

AGREE+SPOT: 4 (map ID and -8 with CommonLibF4AE, BSTHashMap layout, CRC.h, FULL-in-map for six classes). CLAE-ONLY: 0. DIFFERS-PROVEN: 0 (the fork and CommonLibF4AE agree with the engine). UNPROVEN: 0. Spot checks passed: all (layout read from the engine code in three exes); failed: 0.

## Risks

- In-game result UNVERIFIED. The live check (step 4) is the only part that needs a real populated map; it was reasoned from the engine's insert/erase code, not run. A false failure only leaves the group OFF with a log line.
- No lock is taken (OG reads the same way). The apply runs on the main thread after data load.
- 2193216/2267895 bytes: a later AE build that changes either function keeps the group OFF until new bytes are generated.
