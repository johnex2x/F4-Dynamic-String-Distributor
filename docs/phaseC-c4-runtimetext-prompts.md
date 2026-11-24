STATUS: DONE

# Phase C4: runtimeText and prompts on AE

Scripts and raw output: `phaseC/c4/` (a1..g1.py, b3.out, d1.out, ...). Evidence is from the AE exes .191/.221/.240 (own disassembly, RVAs below are .240 unless noted). CommonLibF4AE (CLAE) was used as the comparison source; FallHook was not opened.
All IDs below were checked: each is an exact member of all six 1.11.x bins (137/159/169/191/221/240).

Classes: `AGREE+SPOT`, `AGREE`, `CLAE-ONLY`, `DIFFERS-PROVEN`, `PROVEN` (own evidence, CLAE has no value), `UNPROVEN`.

## Verdicts

| Feature | Verdict | Condition |
|---|---|---|
| `runtimeText` | **READY** | Needs three code changes on AE (below): `BSString::Set` via ID 4471693, `As<TESDescription>` replaced by an RTTI cross-cast with CLAE type-descriptor IDs, form lookup via `DSD::LookupFormByID`. |
| `prompts` | **READY** | Prompt table = global BSTHashMap at data ID 2662549 (replaces the pattern scan); spoken-response ctor 2227131 proven. Needs: byte guard for the ctor, CS pool variant for the stored prompt (see risk R2). |

Header-diff / spot-check summary: CLAE and the fork agree on every TESDescription / TESTopicInfo member in scope. CLAE has no value for BSStringT::Set, TESResponse, GetResponseText, the quest journal functions or the prompt table (those are own evidence). 7 spot checks run, 7 passed, 0 failed.

## 1. runtimeText

### 1a. Description reader (`TESDescription::GetDescription`)

| Item | OG value | AE value | Evidence | Class |
|---|---|---|---|---|
| Function ID | 523613 | **2193019** (RVA 191/221/240 = 0x30E6E0 / 0x30E6F0 / 0x30EA10). CLAE: same ID (`TESDescription::GetDescription`, `BGSLocalizedStringDL::GetDescription`) | Head chunk (pdata size 0x26, rest is chained chunks): `rcx=this`, `rdx=out`, `r8=form`. Body: `[this+8]` fileOffset, `[this+0xC]` chunkOffset, `lea rcx,[this+0x10]` -> localized-string getter 2194238 (0x351B00, = CLAE `BGSLocalizedStringDL::GetText`). First action on the output is `Set(out, nullptr, 0)` (clear), then fills it from the file text. Normalized bytes equal in 191/221/240. | AGREE+SPOT |
| Args are `(TESDescription*, BSString&, const TESForm*)` | same | same | Matches DSD thunk. The 30 direct callers (all 30 in .240) pass `r8 = 0` in every one of the 13 sampled sites (`xor r8d,r8d`) with `rcx = form+0x30 / +0x38 / +0xE8 / ...` (the TESDescription component). The engine then finds the form itself: `__RTDynamicCast(this, 0, SrcType=TESDescription TD, TargetType=TESForm TD, 0)` (0x30EA4A..0x30EA5A). | PROVEN |
| Component-address key is the right key | form->As<TESDescription>() | same idea, but the parent is NOT passed | Because every sampled caller passes a null parent, keying by the parent form is not possible. Keying by the component address (current design) is correct. | PROVEN |
| Hook placement | 5-byte prologue patch | first 5 bytes `40 56 57 41 57` (`push rsi / push rdi / push r15`), no relative operand; return lands on `48 83 EC 40` (`sub rsp,0x40`) | Decoder in `src/HookUtil.cpp` gives length 5 (0x40 REX + 0x56, 0x57, 0x41 0x57). Bytes identical in all three exes up to byte 11; bytes 12..15 are a RIP disp32 (`mov rax,[rip+..]`). | PROVEN |
| Expected-code guard (for `AeHookBytes.h`) | - | 38 bytes: `40 56 57 41 57 48 83 EC 40 48 8B 05 ?? ?? ?? ?? 49 8B F8 4C 8B FA 48 8B F1 80 B8 18 10 00 00 00 0F 85 38 01 00 00` (mask the 4 disp32 bytes at +12..+15) | .191/.221/.240: only bytes +12..+14 differ (the disp32). `jne` rel32 `38 01 00 00` equal in all. | PROVEN |
| `[global+0x1018]` byte at function start | - | if non-zero the function returns without touching `out` | Not identified. Harmless: DSD writes only for components in its table. | UNPROVEN (not needed) |
| Sibling 2193020 (0x30EB80) | - | a second, 5-argument variant (extra stack argument), no direct callers | Same shape, `[this+0x10]` text read. Not hooked by DSD on OG either (the OG contract lists only 523613). Noted, not a blocker. | UNPROVEN (out of contract) |
| TESDescription layout | `fileOffset +8, chunkOffset +0xC, descriptionText +0x10, size 0x18` | same | CLAE `FormComponents.h` and fork agree; the AE body above reads exactly `[this+8]`, `[this+0xC]`, `[this+0x10]`. | AGREE+SPOT |
| Form -> component address, `As<TESDescription>()` | `static_cast` per form type (fork `FormUtil.h`) | **must not be used on AE** (compile-time offsets from the fork layout). Use the engine's own cross-cast: `__RTDynamicCast(form, 0, TD(TESForm), TD(TESDescription), 0)` once per translated form at Apply time (zero cost per hook call). | Type descriptors: CLAE `RTTI::TESForm` = ID **4839494**, `RTTI::TESDescription` = ID **4839902**. They resolve to .data 0x2F90498/0x2F904D8/0x2F9B528 (TESForm) and 0x2F942E8/0x2F94328/0x2F9F378 (TESDescription) in .191/.221/.240. The engine's own call at 0x30EA4A/0x30EA51 loads exactly these addresses (name strings `.?AVTESForm@@` and `.?AVTESDescription@@` read at those RVAs in all three exes). The fork's IDs (2764237 / 2764599) are absent in all six bins. | DIFFERS-PROVEN (CLAE right, fork wrong) |
| `__RTDynamicCast` | 84112 | **4818455** (import thunk `jmp [iat]`) | Called with `edx=0, r8=src TD, r9=target TD, [rsp+0x20]=0` at 0x30EA5A. Phase B proof. | PROVEN |

### 1b. Quest journal reader

| Item | OG value | AE value | Evidence | Class |
|---|---|---|---|---|
| Function ID | 139753 | **2207825** (0x672590 / 0x6725B0 / 0x6728D0), size 0x84 | Re-checked from the body (all three exes): `rcx=quest` (saved in rsi), `rdx=out` (rdi), `r8w=stage` (zero-extended to ebx), `r9b=item` (ebp). Sequence: `Set(out, nullptr, 0)` (clear) -> stage lookup 2207765 (`quest`, `stage`) -> item lookup 2208057 (`stage`, `item`) -> test `[item+0x16]` (has-text flag) -> `GetLogEntry` 2208069 (`item`, `quest`) -> `Set(out, returned c_str, 0)`. Same contract as the DSD thunk (stage u16, item u8). Normalized bytes equal in 191/221/240. | PROVEN (confirms an earlier review) |
| Hook placement | 5-byte prologue | first instruction `48 89 5C 24 08` (`mov [rsp+8],rbx`), 5 bytes, no relative operand; return lands on `48 89 6C 24 10` | Decoder: REX + 0x89 + modrm 0x5C (SIB, disp8) = 5. Bytes identical in all three exes. | PROVEN |
| Expected-code guard | - | first 64 bytes: `48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B FA 41 0F B7 D8 48 8B F1 45 33 C0 48 8B CF 33 D2 41 0F B6 E9 E8 ?? ?? ?? ?? 0F B7 D3 48 8B CE E8 ?? ?? ?? ?? 48 85 C0 74 30 40` (mask = the two rel32 call targets at +43..+46 and +52..+55) | Only the call operand bytes differ between exes. | PROVEN |
| Output write | `BSStringT::Set(c_str, size+1)` | engine `Set` = ID **4471693** (see 1c); the function itself writes through the same `Set` | Body above ends with `Set(out, rax, 0)`. | PROVEN |
| `TESForm::formID` | +0x14 | +0x14 | Phase B (Papyrus `GetFormID`). | PROVEN |
| `GetFormByID<TESQuest>` | fork `GetAllForms` | `DSD::LookupFormByID` (2193092) + check `formType == 0x50` then `static_cast<TESQuest*>` (TESForm is the base at +0, no offset) | `QUST = 0x50`, `INFO = 0x4F` read from the form-type table (ID 1309967, in all six bins) in .191/.221/.240 (`e4.py`). | PROVEN |
| `TESForm::GetFile(int)` | `RelocationID(1376557, 2193103)` | **2193103** (0x3120A0 / 0x3120B0 / 0x3123D0) | Leaf function (no pdata): `r8=[this+8]` (sourceFiles array), `eax=[r8+8]` count, clamp index (negative or >= count -> last), return `[[r8]+idx*8]`. Index 0 = defining plugin, as the DSD code wants. Bytes identical in all three exes. CLAE uses the same ID. | AGREE+SPOT |
| `TESFile::filename` | +0x70 | +0x70 | Phase B. | PROVEN |

### 1c. `BSStringT::Set` and the BSString layout

| Item | OG value | AE value | Evidence | Class |
|---|---|---|---|---|
| `BSStringT<char>::Set(const char*, uint64 maxLen)` | 407804 | **4471693** (0x1E61D0 / 0x1E61D0 / 0x1E64F0), size 0x17D, `this=rcx, str=rdx, maxLen=r8`. Fork slot 2189084 is absent in all six bins. CLAE has no `Set` at all. | Body: `len = strlen(str)` (0 if null); `need = (maxLen ? maxLen : len) + 1`; if `need > capacity [this+0xA]` allocate (`call 0x141657DE0`, the engine allocator) and store the pointer at `[this+0]`, capacity (u16, clamped to 0xFFFF) at `[this+0xA]`; copy `min(len, need-1)` bytes, write NUL, store size u16 at `[this+8]`. Called with `(out, nullptr, 0)` as the "clear" at the start of both readers above. Bytes identical in 191/221/240 (no operand differs inside the first 64). | DIFFERS-PROVEN (ID) |
| Passing `size()+1` as maxLen (current DSD call) | works | works | `need = size+2`, copy length `min(len, size+1)` = `len`. Passing `0` is equivalent and avoids the extra byte. | PROVEN |
| BSStringT layout | `_data +0, _size u16 +8, _capacity u16 +0xA` (fork) | same | `Set` reads/writes exactly these fields. CLAE and fork agree. | AGREE+SPOT |
| Allocator | fork `calloc` (game allocator) | engine allocator `0x141657DE0` | DSD never allocates itself: it only calls the engine `Set` on the engine's own output string, so no allocator replacement is needed and no BSStringT field is written directly. | PROVEN (no action) |

### runtimeText: what the AE code path must change

1. `a_out.Set(...)` -> call a `REL::Relocation<bool(RE::BSStringT<char>*, const char*, std::uint64_t)>{ REL::ID(4471693) }` resolved once; pass `0` as maxLen.
2. `form->As<TESDescription>()` -> `__RTDynamicCast` (ID 4818455) with type descriptors from IDs 4839494 / 4839902 (resolve once at load; the cast runs only in `BuildDescriptions`, never inside a hook).
3. `TESForm::GetFormByID` -> `DSD::LookupFormByID`; typed quest lookup as in 1b; `GetFormTypeString()` in the debug log needs table ID 1309967 (exact in all six bins), otherwise drop it on AE.
4. Add `AeHookBytes` guards for 2193019 and 2207825 (bytes above), then `InstallPrologueHook` as on OG.

## 2. prompts

### 2a. INFO RNAM prompt table

| Item | OG value | AE value | Evidence | Class |
|---|---|---|---|---|
| How OG finds it | pattern `48 83 C6 18 48 83 C7 18` + 0xC | pattern is a destructor loop on AE (rejected, Phase B) | - | - |
| INFO subrecord loader | - | `TESTopicInfo` vtable (RTTI `.?AVTESTopicInfo@@`, vptr 0x24D9C28 in .240) slot 9 = 0x69C320 (ID 2208420) = Load: gets file `r13`, checks record, loops subrecords in 0x69C3A8 with `r15 = info`, `r12 = current response`. A jump table at 0x69CCEC covers `DNAM..TNAM`; entry 14 = `RNAM` -> 0x69C8CE. | Table decoded in `b7.py` (entries: DNAM 0x69C7F9, ENAM 0x69C7E2, GNAM 0x69C81B, INAM 0x69C93B, ... RNAM 0x69C8CE). RNAM case: scratch text init (2194227), read lstring from file (0x3517C0 = ID 2194235), then `insert(map = 0x2F0B460, key = &info, value = scratch)` = ID 2208487 (0x6A11C0). Same code, same targets in .191 (global 0x2F00410) and .221 (0x2F00460). | PROVEN |
| Prompt table location | pattern result | **data ID 2662549 = table base** (RVA 191/221/240 = 0x2F00408 / 0x2F00458 / 0x2F0B458); the engine function receives base+8 = data ID 2662550 (0x2F00410 / 0x2F00460 / 0x2F0B460). IDs 2662549 and 2662550 are exact in all six bins (137: 0x2EEF408, 159: 0x2EF0408, 169: 0x2EF5408). | The only RNAM-keyed table: writers are the INFO loader RNAM case and 0x69E830 (ID 2208447, `SetPrompt(info, const char*)`: non-empty text -> insert, empty -> erase), erase in the INFO destructor (slot 0, 0x6A2250). Readers use the same global. | PROVEN |
| Container type | `BSTHashMap<TESTopicInfo*, BSFixedString>` | same shape | Entry = 0x18 bytes `{key +0, value (BSFixedString) +8, next +0x10}`; an entry is empty when `next == 0`. Insert function (0x6A11C0, receives base+8): capacity u32 at `[rcx+4]` (= base+0xC), free count `[rcx+8]` (base+0x10), good index `[rcx+0xC]` (base+0x14), sentinel `[rcx+0x10]` (base+0x18), entry array `[rcx+0x20]` (base+0x28), index = `hash & (cap-1)`, stride 3*8. In the .data image: 24 zero bytes then the sentinel pointer at base+0x18, exactly the fork `BSTScatterTable` layout (capacity +0xC, free +0x10, good +0x14, sentinel +0x18, entries +0x28). So `*reinterpret_cast<PromptMap*>(base)` works with the fork's iterator unchanged. | PROVEN (layout equals fork) |
| Key | `TESTopicInfo*` pointer value | same | `[rbp-0x41] = r15 (info)`, the hash and compare use the pointer itself. | PROVEN |
| Value pool variant | `RE::BSFixedString` (case-insensitive) in DSD | engine builds the value with the **case-sensitive** pool variant | 0x351390 (ID 2194228, used by `SetPrompt`) and the file path both go through 0x167CD10 = `BSFixedStringCS::operator=(const char*)` (ID 2268697: `GetEntry(.., caseSensitive = 1)`). The CI variant is 0x167C320 (ID 2268671, flag 0). | DIFFERS (fork type CI vs engine CS) - see R2 |
| Map hash function / locking | - | none needed for iteration | DSD only iterates entries at kGameDataReady (OG behavior). | PROVEN |
| TESTopicInfo `formID` | +0x14 | +0x14 | Phase B. | PROVEN |

Replacement for `FindPromptMap()` on AE: `reinterpret_cast<PromptMap*>(REL::ID(2662549).address())` with a guard that (a) the capacity at +0xC is a non-zero power of two, (b) the entries pointer at +0x28 is non-null when capacity > 0, and (c) the 8 bytes at +0x18 equal the sentinel value stored there at load. Resolve once in `ApplyPrompts` (not per call). Note: `ID 2662549` is a data symbol; the function-start checks used for code do not apply, so the bin-exact check plus the layout guard is the safety net.

### 2b. TESTopicInfo / TESResponse members (needed by the response hook and FindResponseOwner)

| Item | OG value | AE value | Evidence | Class |
|---|---|---|---|---|
| `TESTopicInfo.dataInfo` | +0x30 | +0x30 | INFO loader jump-table case `DNAM`: reads a 4-byte form ID and does `mov [r15+0x30], rax` (0x69C80F). Constructor (slot 7, 0x69D380) zeroes +0x20, +0x28, +0x30 and sets `formType` byte +0x1A = 0x4F. CLAE and fork agree. | AGREE+SPOT |
| `TESTopicInfo.data` (flags) | +0x44 | +0x44 | case `ENAM` reads 4 bytes into `[r15+0x44]`; the response ctor reads `word [info+0x44]` bit 0xB. | AGREE+SPOT |
| `TESTopicInfo.responses.head` | +0x48 | +0x48 | Load case `TRDA` appends a new 0x18-byte `TESResponse` to the list at `[r15+0x48]` (head) and links `[tail+8]`; the destructor calls the list dtor on `lea rcx,[this+0x48]`. | AGREE+SPOT |
| `TESResponse` size / `pNext` | 0x18 / +8 | 0x18 / +8 | `TRDA` case allocates 0x18 bytes (`lea edx,[r9+0x18]`), link write `mov [rcx+8], r12`. | PROVEN |
| `TESResponse.data` | percent u16 +0x10, responseID u8 +0x12, useEmotion +0x13 | same | `TESResponse` TRDA loader (0x6937C6): `mov [rbx+0x12], r12b` (responseID), `mov [rbx+0x10], si` (percent), `mov [rbx+0x13], al` (useEmotion). | PROVEN |
| `TESResponse.faceArchetype` | +0x14 | +0x14 | Same loader: `mov [rbx+0x14], ax`; getter 0x693F90 reads `word [this+0x14]`. | PROVEN |
| `TESResponse.responseText` (BGSLocalizedString) | +0 | +0 | `GetResponseText` ID 2208288 (0x693C30, a `jmp` thunk to 0x352100) passes `rcx` straight through and returns the c_str of the string at +0. ID 2208288 equals the fork's second slot. | PROVEN |

### 2c. Spoken-response constructor

| Item | OG value | AE value | Evidence | Class |
|---|---|---|---|---|
| ID | 755245 | **2227131** (0xBAE830 / 0xBAE6A0 / 0xBAE620), size 0x273 (OG 0x47) | Only one direct caller (0xBADA5E, in an unnamed function). Normalized bytes equal in 191/221/240. | PROVEN |
| Argument order | `(this, topic, info, speaker, response, quest)` | same | `rcx -> rsi` (this), `rdx -> r12` (topic, passed as 4th argument to 0x694A90), `r8 -> rdi` (info: `word [rdi+0x44]` = TOPIC_INFO flags, bit 0xB tested), `r9 -> r14` (speaker, passed to 0x4F4850), 5th argument `[rsp+0x1A0]` -> `rbp` (response: `[rbp+0x10]`, `[rbp+0x13]`, getters 0x693C30 / 0x693F90 / ...), 6th `[rsp+0x1A8]` -> `rbx` (quest, passed to 0x671FA0). 0x1A0 = 0x28 (shadow + return) + 5 pushes + 0x150. | PROVEN |
| Return value | `this` | `this` (`mov rax, rsi` at the end) | 0xBAE87C. | PROVEN |
| Text field | `BSFixedStringCS` at +0 | same | 0xBAE691: `call 0x14167CD10` (CS assign) with `rcx = this`, `rdx = GetResponseText(response)`. Object size 0x40: fields up to `[this+0x3A]` (byte). Fork `result->text = *translation` goes through `GetEntry` (2268729) with the CS flag and `Entry::release` (2268720): the same calls the engine makes in 0x167CD10 (0x167DD20 and 0x167D4A0). IDs exact in all six bins. | PROVEN |
| Hook placement | 5-byte prologue | first instruction `48 89 5C 24 10` (`mov [rsp+0x10], rbx`), 5 bytes, no relative operand; return lands on `48 89 6C 24 18` | Bytes identical in all three exes. | PROVEN |
| Expected-code guard | - | first 64 bytes: `48 89 5C 24 10 48 89 6C 24 18 56 57 41 54 41 56 41 57 48 81 EC 50 01 00 00 4D 8B F1 49 8B F8 4C 8B E2 48 8B F1 E8 ?? ?? ?? ?? 33 DB 48 8D 4E 18 48 89 5E 08 66 89 5E 10 E8 ?? ?? ?? ?? 48 8B AC` (mask the rel32 at +38..+41 and +57..+60) | Only those operand bytes differ. | PROVEN |
| `DialogueResponse` mirror (`text` +0 only) | `BSFixedStringCS` | same | The DSD struct only declares offset 0; the constructor leaves it as the first member (this+0). | PROVEN |

## Risks and notes for the implementer

- R1. Description keys: the table is built with the component address of each translated form. If a plugin form type has no TESDescription base the cross-cast returns null (log it as "no description", as now).
- R2. Prompt value pool: the engine stores prompts with the case-sensitive pool variant. `prompt = entry->text` through the fork's `BSFixedString` (CI) uses the case-insensitive pool lookup, which can return an existing entry that differs only in case. On AE declare the map value as `RE::BSFixedStringCS` (same layout, size 8) so the assignment uses `GetEntry(.., true)` like the engine. Whether OG uses CS too is UNPROVEN (OG exe is encrypted); do not change OG.
- R3. The function at 2193019 skips its work when a global byte is set (see table); DSD's thunk still writes the translation after the call. This matches the OG thunk and is not a regression.
- R4. `ApplyPrompts` iterates the live table; this is unchanged from OG (no lock).

## Counts

| Class | Rows |
|---|---|
| AGREE+SPOT | 10 (GetDescription ID, TESDescription layout, GetFile 2193103, BSStringT layout, dataInfo +0x30, data +0x44, responses +0x48, plus RTTI IDs 4839494 / 4839902 counted in DIFFERS-PROVEN) |
| DIFFERS-PROVEN | 3 (BSStringT::Set ID 2189084 -> 4471693; fork RTTI IDs 2764237/2764599 -> CLAE 4839494/4839902 for the cross-cast; fork `As<>` static offsets -> RTTI cross-cast) |
| DIFFERS (open choice) | 1 (prompt value pool CS vs CI, R2) |
| PROVEN (own evidence, no CLAE value) | 24 |
| UNPROVEN | 3 (global byte at function start, sibling 2193020, OG pool variant) |

Spot checks: 7 passed, 0 failed (TESDescription layout, GetDescriptionOwnerFile = vslot 0x18 through `[rax+0xC0]`, GetFile 2193103 body, GetText 2194238 = 0x351B00, CLAE RTTI IDs 4839494/4839902 = engine's own `lea` operands, TESTopicInfo dataInfo / data / responses offsets, TESResponse layout).
