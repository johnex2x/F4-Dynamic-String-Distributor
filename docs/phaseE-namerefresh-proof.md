# Phase E — NameRefresh on Fallout 4 AE

Scope: `1.11.191`, `1.11.221`, `1.11.240`; static x64 analysis, not an in-game test. The machine-code evidence below comes from those three executables. Address Library IDs label locations (or are tested for exact membership in fact 12); their presence alone does not establish function identity. The fork headers define the comparison, not the AE result. No other mod's source was used as evidence. All addresses are RVAs, in `.191 / .221 / .240` order unless otherwise specified.

Existing evidence, not redone: [Phase B](ae-crosscheck.md) for `TESObjectREFR.extraList +0x100`, its base object `+0xE0`, and the location and first `FindExtra(0x99)` call of 2190179; [Phase C](phaseC-ae-id-audit.md) for `lock_read` 2267897 and DSD's DataHandler singleton 4796135; [Phase D](phaseD-dialogue-options.md) for the player singleton 2698073. Phase E extends the first-call finding to the behavior of 2190179.

| # / Fact | Fork assumption | AE .191 | AE .221 | AE .240 | Evidence | Verdict |
|---|---|---|---|---|---|---|
| 1. SetDisplayNameFromInstanceData | `457340/2190179`; `(list, object-instance extra, base, smart-pointer const&)`, `void`; might remove the text extra | 2190179 @ `0x27E5E0` | 2190179 @ `0x27E5E0` | 2190179 @ `0x27E900` | §1: `r9` is indirectly loaded; existing text can be edited in place or removed, absent text added | PROVEN |
| 2. Extra type codes | Text `0x99`, object instance `0x35`, instance data `0xBA` | `0x99 / 0x35 / 0xBA` | same | same | §2: typed constructors and callers of the common lookup | PROVEN |
| 3. ExtraTextDisplayData | Fixed string `+0x18`, message `+0x20`, quest `+0x28`, flags `+0x10` bit 0 = custom | `+0x18 / +0x20 / +0x28 / +0x10:0` | same | same | §3: RTTI, constructor, field users and custom-name setter | PROVEN |
| 4. ExtraInstanceData | Base `+0x18`; intrusive data pointer `+0x20` | `+0x18 / +0x20` | same | same | §4: RTTI construction and data-pointer users | PROVEN |
| 5. BGSObjectInstanceExtra | Values pointer `+0x18` and index-data layout | `+0x18`; buffer pointer `+0`, payload size `+8`, block 0 trailer at payload end; `0x8`-byte records: ID/index/rank/disabled `+0/+4/+5/+6` | same | same | §5: constructor, block decoder, record reader and writer | PROVEN |
| 6. REFR inventory list | Pointer at `+0xF8` | `+0xF8` | same | same | §6: REFR reader and allocator | PROVEN |
| 7. Inventory list, item and stack | Array `+0x58` (pointer `+0`, capacity `+8`, size `+0x10`); lock `+0x78`; item stride `0x10`, object `+0`, stack head `+8`; stack next `+0x10`, extra `+0x18`, count `+0x20` | fork values | same | same | §7: inventory iterator, array growth, stack traversal and lifetime | PROVEN |
| 8. CELL references and spin lock | References `+0x70`; lock `+0xC0`; `lock` engine call `1425657/2192245`, `unlock` inline | `+0x70` (data/capacity/size `+0x70/+0x78/+0x80`); spin lock `+0xC0`; 2192245 @ `0x2DD4E0` | same | same fields; 2192245 @ `0x2DD800` | §8: locked reference-array use and spin-lock code | PROVEN |
| 9. DataHandler arrays | `formArrays +0x68`, BSTArray stride `0x18`, `kWRLD=0x4A`; `interiorCells +0xF58` with NiTPrimitiveArray layout | `+0x68`, stride `0x18`, `WRLD=0x4A` → `+0x758`; Ni array `+0xF58`, pointer relative `+8`, u16 capacity/high-water/count/grow `+0x10/+0x12/+0x14/+0x16` | same | same | §9: DataHandler construction, form indexing, WRLD RTTI/type, interior insertion and compaction | PROVEN (layout; see sparse-walk caveat) |
| 10. WorldSpace cells | Inline BSTHashMap `+0x40`; persistent cell `+0x70` | Map `+0x40`; world-relative capacity/free/good/sentinel/entries `+0x4C/+0x50/+0x54/+0x58/+0x68`; entry stride `0x18`; persistent `+0x70` | same | same | §10: cell insert and map iterator; matches live fork fields, not a claim about padding | PROVEN |
| 11. Read unlock | Atomic decrement of 32-bit lock word at `+4` | `lock dec dword ptr [rcx+4]` | same | same | §11: matched read acquire/release on one lock | PROVEN |
| 12. Debug IDs (membership only) | Type strings `1309967`; `LookupForm` → form map `422985/2689178`, map lock `691815/2689189` | Type strings `0x2ED0B60`; map and lock IDs absent | same | Type strings `0x2EDBB60`; map and lock IDs absent | §12: exact bin lookup (including the three earlier 1.11.x bins) | PROVEN for type-string ID; DIFFERS for `LookupForm` dependencies |

## Changes DSD needs on AE

- Keep `DSD::GetDataHandler()` and `DSD::GetPlayer()` on the main path. Keep the `LookupForm` holster trace and OG-ID `457340` prologue probe OG-only. The type-string ID is present; absence is not a reason to disable that log path.
- Walk interior-cell slots to the validated first-free/high-water index and skip nulls (or otherwise establish a dense array before using `size()`). `size()` is occupied count, not a bound on the last occupied index. No struct-offset change is required for this.
- Use `2190179` with the smart-pointer reference, keep the guard for custom/message/quest names, and re-fetch the text extra after the call: the old node can be freed. Start with the existing dry run before enabling writes. Do not generalize this proof to uninspected AE executables.

## Not proven

- These are ABI/layout witnesses, not recovery of source-level declarations: e.g., machine code confirms what is loaded from the fourth argument, not the C++ `const` spelling. Unused map padding and other unaccessed bytes were not identified.
- Fact 12 does not prove the debug helper implementations; it checks the actual IDs they use.

No in-game verification of the refresh sequence, its timing or interaction with other code was performed. Bin membership for `.137`, `.159`, and `.169` is not an executable proof for those versions.

## Instruction evidence

### Extra-data ID/RVA key

All addresses are RVAs (hex). Instruction offsets in the evidence below are relative to the owning RVA in this table, and the listed offsets were checked in all three executables. `+X → ID` gives the resolved call/code or RTTI/data target, not the encoded displacement.

| ID | Role | 191 | 221 | 240 |
|---|---|---:|---:|---:|
| 2190179 | SetDisplayNameFromInstanceData | `27E5E0` | `27E5E0` | `27E900` |
| 2190168 | erase quest/message display text; restore instance name | `27DB30` | `27DB30` | `27DE50` |
| 2190185 | look up/create instance data; optionally regenerate display name | `27EB90` | `27EB90` | `27EEB0` |
| 2189033 | attach mod to reference | `248F40` | `248F40` | `249260` |
| 2190936 | locked `ExtraDataList` lookup | `2A0030` | `2A0030` | `2A0350` |
| 2190085 | base-list lookup | `272650` | `272650` | `272970` |
| 2189035 | construct generated name into temporary output, return success in `al` | `249320` | `249320` | `249640` |
| 2190971 | locked remove-by-type | `2A15B0` | `2A15B0` | `2A18D0` |
| 2190083 | base-list unlink | `272540` | `272540` | `272860` |
| 2267872 | allocator | `16579C0` | `1657AC0` | `1657DE0` |
| 2267874 | deallocator | `1657E20` | `1657F20` | `1658240` |
| 2190758 | extra-text constructor with message/quest inputs | `29C7D0` | `29C7D0` | `29CAF0` |
| 2190759 | default extra-text constructor | `29C850` | `29C850` | `29CB70` |
| 2190907 | locked add | `29F5F0` | `29F5F0` | `29F910` |
| 2190080 | base-list insertion | `272380` | `272380` | `2726A0` |
| 2191320 | assign literal/generated string to text extra | `2AC780` | `2AC780` | `2ACAA0` |
| 2268692 | fixed-string constructor | `167C770` | `167C870` | `167CB90` |
| 2268697 | case-sensitive fixed-string assignment | `167C8F0` | `167C9F0` | `167CD10` |
| 2268729 | intern string with case-preserving flag | `167D900` | `167DA00` | `167DD20` |
| 2190167 | set explicit/custom name | `27DA50` | `27DA50` | `27DD70` |
| 2191325 | extra-text load and resolve form pointers | `2ACAD0` | `2ACAD0` | `2ACDF0` |
| 4818455 | dynamic cast | `22BCB9E` | `22BCC7E` | `22C46AE` |
| 4839494 | `.?AVTESForm@@` type descriptor | `2F90498` | `2F904D8` | `2F9B528` |
| 4839780 | `.?AVBGSMessage@@` type descriptor | `2F93610` | `2F93650` | `2F9E6A0` |
| 4840074 | `.?AVTESQuest@@` type descriptor | `2F95410` | `2F95450` | `2FA04A0` |
| 34112 | `.?AVExtraTextDisplayData@@` primary vtable | `2468700` | `2468700` | `2470760` |
| 179124 | `.?AVExtraInstanceData@@` primary vtable | `246A070` | `246A070` | `24720D0` |
| 137209 | `.?AVBGSObjectInstanceExtra@@` primary vtable | `2462298` | `2462298` | `246A2F8` |
| 2191406 | default instance-data constructor | `2B1D10` | `2B1D10` | `2B2030` |
| 2191407 | instance-data constructor with base and data | `2B1D40` | `2B1D40` | `2B2060` |
| 2189015 | object-instance constructor | `2471A0` | `2471A0` | `2474C0` |
| 2190180 | walk type-0x35 index block to find mod | `27E730` | `27E730` | `27EA50` |
| 2189032 | resolve attachment mod from FormID | `248F10` | `248F10` | `249230` |
| 2193092 | resolve form from FormID | `311850` | `311860` | `311B80` |
| 2189025 | AddMod | `2480F0` | `2480F0` | `248410` |

### 1. Calling convention, actual branches, and entry bytes

[Earlier crosscheck](ae-crosscheck.md) establishes 2190179's anchor and the first `FindExtra(0x99)`. Here the complete 2190179 body is `[entry,entry+0x144)`: `.pdata` reports `[+0,+55)`, `[+55,+103)`, `[+103,+144)`, with the latter two unwind entries chained to the first. `.240` returns at `27EA43`, with `27EA44` beginning the padding, not part of the body.

| 2190179 offset | Instruction / effect |
|---|---|
| `+12`, `+15`, `+18`, `+1A`, `+1D`, `+20` | `mov r14,rdx`; `mov rsi,r9`; `mov dl,0x99`; `mov rbp,r8`; `mov rdi,rcx`; `call 2190936` (`rcx=ExtraDataList*`, untouched into lookup). |
| `+25`, `+28`, `+2B` | `mov rbx,rax`; `test rax,rax`; `je +4D` (when text extra absent). |
| `+2D/+31`, `+37/+3C`, `+42/+47` | `test byte ptr [rax+0x10],1`; `jne +131`; `cmp qword ptr [rax+0x20],0`; `jne +131`; `cmp qword ptr [rax+0x28],0`; `jne +131`. A custom name, message, or quest returns untouched. |
| `+4D`, `+50`, `+5A`, `+60`, `+6D` | `mov r8,qword ptr [rsi]`; `lea r9,[rsp+0x20]`; `mov rdx,rbp`; `mov rcx,r14`; `call 2189035`. The incoming **r9 points to a pointer word**; it is dereferenced once, then the contained instance-data pointer is passed along. The incoming `rdx` is the object-instance extra (`2189035+25: cmp word ptr [rcx+0x20],ax`, reading its item index, after `+14: mov eax,0xffff`); incoming `r8` is the base object (caller provenance below). The outgoing `r9` is the address of a different, stack-local name-output temporary. |
| `+72`, `+74`, `+76`, `+79` | `test al,al`; `je +EA`; `test rbx,rbx`; `jne +CD`. Success with a preexisting unguarded extra jumps straight to assignment, not allocation or removal. |
| `+A4`, `+A8`, `+B2`, `+B5`, `+BA`, `+C2`, `+C5`, `+C8` | Success without an extra: `lea edx,[r9+0x48]` (`r9=0` at `+97`); `call 2267872`; when allocation succeeds, `mov rcx,rax`; `call 2190759`; `mov rbx,rax`; `mov rdx,rbx`; `mov rcx,rdi`; `call 2190907`. Add delegates at `+28: call 2190080`; its `+43: mov qword ptr [rax],rdi` links the new node (the 0x99 case follows tail insertion). |
| `+CD`, `+DC`, `+DF`, `+E3`, `+E8` | `mov rax,qword ptr [rsp+0x20]`; `mov rcx,rbx`; `cmovne rdx,rax` (otherwise `rdx` is the fallback string loaded at `+D2`); `call 2191320`; `jmp +F4`. The setter changes the string at +0x18 on the same extra. |
| `+EA`, `+EC`, `+EF` | Failed synthesis: `mov dl,0x99`; `mov rcx,rdi`; `call 2190971`. Remove-by-type `+5C: call 2190083`; unlink checks type (`+59: cmp byte ptr [r8+0x12],bl`), splices the next link into the predecessor (`+6F: mov qword ptr [r9],rcx`) and clears the removed node's next (`+72: mov qword ptr [rdx],rsi`, `rsi=0`). `2190971+6C: mov edx,1`; `+71: call qword ptr [rax]` invokes the removed node's deleting destructor if one was unlinked. No replacement on this branch. |
| `+F4`, `+FE`, `+101`, `+12C`, `+131` … `+143` | `mov rbx,qword ptr [rsp+0x20]`; `test rbx,rbx`; `je +131`; else free temporary via `call 2267874` at `+12C`. The shared epilogue begins with `mov rbx,qword ptr [rsp+0x58]`, restores registers/stack, and ends in `ret` at `+143`. The early guard path leaves the lookup pointer in `rax`; other paths leave incidental callee results. There is no common result contract. |

**Call sites:** In 2190168 the lookups return an object-instance extra (saved to `rdi`) and an instance-data extra (`rax`); `+CD: lea r9,[rax+0x20]`, `+D1: mov r8,r12`, `+D4: mov rdx,rdi`, `+D7: mov rcx,r15`, `+DA: call 2190179`. The caller restores registers and writes its own saved value at `+E9: mov dword ptr [rsi],ebx` without using `rax`. In 2189033 the proven reference base offset ([earlier crosscheck](ae-crosscheck.md)) is read at `+26: mov rax,qword ptr [rcx+0xe0]`, saved at `+33: mov qword ptr [rsp+0x60],rax`, restored at `+11D: mov r12,qword ptr [rsp+0x60]`, and passed into 2190185 at `+21F: mov rdx,r12; +222: call 2190185`. In 2190185 `+46: mov r15,rdx` retains that base, and `+15E: mov rcx,qword ptr [rsp+0x60]`, `+163: lea r9,[rsp+0x78]`, `+168: mov r8,r15`, `+16B: mov rdx,rbp`, `+16E: call 2190179` uses it. This caller next reloads its smart-pointer temporary at `+173: mov rcx,qword ptr [rsp+0x78]`, not `rax`, and does reference-count clean-up before preparing its own result. This establishes the void ABI and read-only reference to the smart-pointer storage; source-level `const` spelling is not encoded separately in these machine instructions.

The following 64 bytes are identical at `27E5E0` (191), `27E5E0` (221), and `27E900` (240):

```text
+00  48 89 5c 24 10 48 89 6c 24 18 56 57 41 56 48 83
+10  ec 30 4c 8b f2 49 8b f1 b2 99 49 8b e8 48 8b f9
+20  e8 2b 1a 02 00 48 8b d8 48 85 c0 74 20 f6 40 10
+30  01 0f 85 fa 00 00 00 48 83 78 20 00 0f 85 ef 00
```

**Differing rel32 / RIP-relative fields in this window: none.** The call rel32 bytes `+21..+24` (`2b 1a 02 00`) resolve to 2190936 in all three. The `jne` rel32 at `+33..+36` (`fa 00 00 00`) and `+3e..+41` (`ef 00 00 00`, two bytes outside the requested window) both resolve to `entry+0x131`; their encodings do not change. `+2B` is a short relative branch, not rel32. There are no RIP-relative memory operands in the first 0x40 bytes.

### 2–4. Type tags, text-extra fields, instance-data fields

Each RTTI vtable in the address key is tied to its type by the pointer at `vtable[-1]` to the Complete Object Locator and the locator's type descriptor (`+0xC`, name at descriptor `+0x10`). The constructors below install those vtables, not just similarly shaped objects.

- **0x99:** 2190759 `+0A: xor edi,edi`; `+0C: mov byte ptr [rcx+0x12],0x99`; `+10: lea rax,[rip+disp] → 34112`; `+22: mov qword ptr [rcx],rax`. 2190179 `+18: mov dl,0x99` and `+20: call 2190936` retrieve that type and consume its flags and message/quest fields. 2190167 uses the same lookup.
- **0x35:** 2189015 `+26: mov byte ptr [rcx+0x12],0x35`; `+2E: lea rax,[rip+disp] → 137209`; `+40: mov qword ptr [rcx],rax`. 2190168 `+AA: mov dl,0x35`; `+AC: lea rcx,[r15+8]`; `+B0: call 2190085`; `+BB: mov rdi,rax` saves it for the `rdx` input of 2190179. 2190180 also looks it up and immediately fetches its values pointer.
- **0xBA:** 2191407 `+02: mov byte ptr [rcx+0x12],0xba`; `+0E: lea rax,[rip+disp] → 179124`; `+15: mov qword ptr [rcx],rax`. 2190168 `+B5: mov dl,0xba`; `+B7: lea rcx,[r15+8]`; `+BE: call 2190085`; `+C8: test rax,rax`; `+CB: je +DF`; `+CD: lea r9,[rax+0x20]` takes the address of this extra's smart pointer for the regeneration call.
- **Text string +0x18:** 2190759 `+25: add rcx,0x18`; `+29: call 2268692` constructs the fixed string. 2191320 `+1E: add rcx,0x18`; `+22: call 2268697` assigns to it. The assignment reads the pointer word (`2268697+06: mov rax,qword ptr [rcx]`), selects case-sensitive string interning (`+16: mov r8b,1`; `+19: call 2268729`), and stores zero on the null-string path (`+31: mov qword ptr [rcx],0`). The interner saves that flag (`+22: movzx esi,r8b`), then `+3C: test sil,sil`; `+49: je +88` takes the case-folding path only when it is zero. The selected hash path feeds the original byte (`+62: movsx ecx,r9b`) directly into the hash; the alternate path has `+98: call qword ptr [rip+disp]` targeting the `tolower` IAT slot.
- **Message +0x20 and quest +0x28:** The default ctor 2190759 clears both (`+31: mov qword ptr [rbx+0x20],rdi`; `+35: mov qword ptr [rbx+0x28],rdi`, `rdi=0`). The parameterized ctor 2190758 installs vtable at `+32: mov qword ptr [rcx],rax` (loaded at `+20`), saves `rdx` in `rbx` and `r8` in `rdi`, and at `+47: mov qword ptr [r14+0x20],rbx`; `+53: mov qword ptr [r14+0x28],rdi` stores them. Types are proven by 2191325: at `+47: lea r9,[rip+disp] → 4839780` and `+50: lea r8,[rip+disp] → 4839494`, then `+57: call 4818455`; stores the `TESForm`→`BGSMessage` cast result via `+63: mov qword ptr [r14+0x20],rax`. Repeats with `+87: lea r9,[rip+disp] → 4840074`, `+90: lea r8,[rip+disp] → 4839494`, `+97: call 4818455`; stores the `TESForm`→`TESQuest` result via `+A2: mov qword ptr [r14+0x28],rax`. Both are read by 2190179 and by 2190168 (`+60: cmp qword ptr [rax+0x20],r14`; `+66: cmp qword ptr [rax+0x28],rdi`) in its matching-message/quest removal path.
- **Flags +0x10 bit 0:** 2190759 `+1B: mov word ptr [rcx+0x10],di`, with `di=0`; 2191325 `+DB: or word ptr [r14+0x10],cx` merges persisted flags. 2190167 accepts a literal string (`+13: cmp byte ptr [rdx],0`; `+16: mov rsi,rdx`), looks up 0x99 at `+22/+29` and obtains or constructs the node, passes the saved string (`+AB: mov rdx,rsi`) into `+B1: call 2191320`, and at `+B6: or word ptr [rdi+0x10],1` marks the explicitly supplied name. 2190179 branches away on that exact bit before regeneration. This bit is distinct from the `+0x30 == -2` marker written by the shared string setter (2191320 `+3D: mov dword ptr [rbx+0x30],0xfffffffe`), which occurs even for synthesized names.
- **Instance data +0x18/+0x20:** Default ctor 2191406 zeroes both (`+00: xor edx,edx`; `+1B: mov qword ptr [rcx+0x18],rdx`; `+1F: mov qword ptr [rcx+0x20],rdx`). Parameterized 2191407 `+1B: mov qword ptr [rcx+0x18],rdx` stores the same base pointer passed to regeneration (`2190185+139: mov rdx,r15` followed by `+13F: call 2191407`). `2190185+134: mov r8,qword ptr [rsp+0x78]` supplies its instance data; `2191407+1F: mov qword ptr [rcx+0x20],r8`; `+23: test r8,r8`; `+26: je +2D`; `+28: lock inc dword ptr [r8+8]` establishes intrusive ownership. 2190185 decrements the temporary (`+17D: mov eax,0xffffffff`; `+182: lock xadd dword ptr [rcx+8],eax`) and gets the data pointer from the extra (`+19B: mov r14,qword ptr [rsi+0x20]`). See the `lea r9,[rax+0x20]` use in 2190168 above.

The flag also changes equality, not just hashing. Pool interner 2268729 saves `r8b` at `+0: mov byte ptr [rsp+0x18],r8b`, then `+0xC7 movzx ebx,byte ptr [rbp+0x77]` reloads it, and `+0x151 movzx r9d,bl; +0x160 call 2268727` forwards it. Matcher **2268727** (`0x167D6C0 / 0x167D7C0 / 0x167DAE0`) uses `+0x15 movzx esi,r9b; +0x35 test sil,sil; +0x38 je +0x91`. With the flag set it compares raw bytes (`+0x70 movzx edx,byte ptr [rax]; +0x73 movzx ecx,byte ptr [rax+r8]; +0x78 sub edx,ecx`); the zero-flag branch instead calls the `_stricmp` IAT slot at `+0xBC` (slot RVA `0x24395E8 / 0x24395E8 / 0x24415E8`).

### 5. Object-instance index block

- **Extra and `BSTDataBuffer<1>`:** 2189015 `+23: xor r12d,r12d`; `+46: mov qword ptr [rcx+0x18],r12` initializes the values pointer after installing the object-instance vtable. 2190180 `+04: mov dl,0x35`; `+0F: call 2190936`; `+1D: mov rax,qword ptr [rax+0x18]` loads that pointer. `+39: mov r9,qword ptr [rax]` loads the buffer; `+50: mov ecx,dword ptr [rax+8]`; `+53: add rcx,r9` computes the trailing block descriptor address. `+60: movzx eax,byte ptr [rcx+3]`; `+64: test al,al`; `+66: je +11E` selects ID 0. `+6C: cmp al,0xff`; `+6E: je +85` recognizes the sentinel. The nonmatching block path reads the descriptor word (`+70: mov eax,dword ptr [rcx]`), masks to its size (`+74: and eax,0xffffff`), steps four bytes (`+79: add rcx,4`), adds to data offset (`+7D: add r8d,eax`) and stops after one descriptor (`+80: cmp edx,1`; `+83: jb +60`). The hit path reads the descriptor (`+11E: mov esi,dword ptr [rcx]`), forms data pointer from accumulated offset (`+120: mov r14d,r8d`; `+123: add r14,r9`), and computes `(descriptor & 0xffffff) / 8` by `+126: shr esi,3`; `+129: and esi,0x1fffff`. For ID 0 in a one-block buffer the offset is zero.
- **Record fields:** 2190180 `+A0: mov ecx,dword ptr [r14+rbx*8]`; `+A4: lea rdi,[r14+rbx*8]`; `+A8: call 2189032` resolves the record's FormID (that helper calls the form resolver 2193092 at `+08` and checks a mod type tag at `+12: cmp byte ptr [rax+0x1a],0x90`). `2190180+B5: cmp byte ptr [rdi+6],0`; `+BF: jne +F5` skips the enabled-mod path for a nonzero disabled byte. 2189033 passes the attach-index and rank bytes to the `AddMod` anchor (`+105: movzx r9d,sil`; `+10E: movzx r8d,r12b`; `+118: call 2189025`); 2189025 saves `r9b` and `r8b` at `+00/+05` and writes them at `+2C2: movzx eax,byte ptr [rsp+0x98]`; `+2D4: mov byte ptr [rdi+rcx*8+5],al`; `+2D8: movzx eax,byte ptr [rsp+0x90]`; `+2E0: mov byte ptr [rdi+rcx*8+4],al`. The mod's FormID and initial disabled state are written by `+2EC: mov eax,dword ptr [rax+0x14]`; `+2EF: mov dword ptr [rdi+rcx*8],eax`; `+2F2: mov byte ptr [rdi+rcx*8+6],sil` (`esi` zeroed at `+6F` on this path).

No ABI/layout change is needed for facts 1–5. Treat the old text-extra pointer as invalid after regeneration: failure may delete it, so look it up again before reading its fields. This proves layout, not that every valid BGSObjectInstanceExtra has a nonnull `values`/buffer; the engine scan checks both.

### Inventory and read-lock ID/RVA key

These are RVAs, not preferred-base VAs. Below, `+offset` is relative to the specified function's RVA; instructions and referenced IDs are the same in all three builds. RIP-relative and direct branch operands are annotated with their resolved targets.

| Address Library ID | Use | 191 | 221 | 240 |
|---|---|---:|---:|---:|
| 179707 | RTTI `.?AVTESObjectREFR@@` primary vtable | `0x249cbc8` | `0x249cbc8` | `0x24a4c28` |
| 2200944 | REFR virtual inventory operation (slot 114) | `0x5025e0` | `0x5025f0` | `0x502910` |
| 2201116 | REFR virtual inventory creation (slot 167) | `0x511f10` | `0x511f20` | `0x512240` |
| 2194153 | Inventory-list constructor | `0x34d320` | `0x34d330` | `0x34d650` |
| 2194167 | List item scan | `0x34db20` | `0x34db30` | `0x34de50` |
| 2194191 | List insertion | `0x34ef60` | `0x34ef70` | `0x34f290` |
| 2267911 | Array append-slot helper | `0x1659520` | `0x1659620` | `0x1659940` |
| 1402049 | Array allocator callback vtable | `0x245ffb0` | `0x245ffb0` | `0x2468010` |
| 2188663 | Allocator callback slot 0 | `0x1e7bb0` | `0x1e7bb0` | `0x1e7ed0` |
| 2188699 | Allocator callback slot 1 | `0x1e7ba0` | `0x1e7ba0` | `0x1e7ec0` |
| 2267916 | Initial allocation | `0x16597d0` | `0x16598d0` | `0x1659bf0` |
| 2267917 | Growth allocation | `0x1659850` | `0x1659950` | `0x1659c70` |
| 2267872 | Heap allocator | `0x16579c0` | `0x1657ac0` | `0x1657de0` |
| 938404 | RTTI `.?AVStack@BGSInventoryItem@@` primary vtable | `0x2472168` | `0x2472168` | `0x247a1c8` |
| 2194062 | Item/Stack construction | `0x349650` | `0x349660` | `0x349980` |
| 2194071 | Item count across stacks | `0x349b30` | `0x349b40` | `0x349e60` |
| 2194079 | Item display-name lookup | `0x349e70` | `0x349e80` | `0x34a1a0` |
| 2194138 | Stack destructor (slot 0) | `0x34c860` | `0x34c870` | `0x34cb90` |
| 2190164 | Text-display extra lookup wrapper | `0x27d8c0` | `0x27d8c0` | `0x27dbe0` |
| 2190936 | `FindExtra` | `0x2a0030` | `0x2a0030` | `0x2a0350` |
| 2267897 | `lock_read` (see Phase C ID audit) | `0x1658fe0` | `0x16590e0` | `0x1659400` |
| 2267898 | `lock_write` | `0x1659060` | `0x1659160` | `0x1659480` |
| 2267903 | `unlock_read` | `0x16592b0` | `0x16593b0` | `0x16596d0` |

### 6. REFR inventory list

On the AE `TESObjectREFR` primary vtable, slot 114 (`+0x390`) is `2200944`:

```asm
+00 mov rcx, qword ptr [rcx + 0xf8]
+07 test rcx, rcx
+0a jne 2194167
+10 ret
```

Slot 167 (`+0x538`) is `2201116`: `+0x19 mov rbx, rcx` saves the REFR; `+0x3e mov edx, 0x80; +0x43 call 2267872` allocates the list; alternate paths call its constructor `2194153` at `+0x73` and `+0x92`; `+0x9b mov qword ptr [rbx + 0xf8], rax` installs it. The constructor preserves `this` (`+0x16 mov rsi, rcx`) and returns it (`+0xb9 mov rax, rsi`); its write-lock use below identifies the same list layout as the reader.

### 7. Inventory array, lock and stack

In `2194167`, the list is saved by `+0x18 mov rsi, rcx`, and the sought object by `+0x22 mov rdi, rdx`. Iteration reads the 32-bit element count from `+0x68` and walks from data at `+0x58` to `data + (size << 4)`. The first qword of each 16-byte item is compared to the sought object:

```asm
+2a mov eax, dword ptr [rsi + 0x68]
+31 mov rcx, qword ptr [rsi + 0x58]
+35 shl rax, 4
+39 add rax, rcx
+41 cmp qword ptr [rcx], rdi
+46 add rcx, 0x10
```

In `2194191`, `+0x26 mov r14, rcx; +0x50 lea r13, [r14 + 0x58]` gives the array address. At `+0x4e1 mov r8d, dword ptr [r13 + 8]`, `r8d` gets its capacity (`list+0x60`); `+0x4e5 lea rcx, [r13 + 0x10]` gives the size field address (`list+0x68`). `+0x4ec mov qword ptr [rsp + 0x50], r13` saves the array in the allocator adapter; `+0x4f1 lea rax, [rip + disp32]` selects vtable `1402049`; `+0x4f8 mov r9d, 0x10` passes the item size; `+0x4fe lea rdx, [rsp + 0x48]; +0x503 mov qword ptr [rsp + 0x48], rax; +0x508 call 2267911` invokes append. The append helper compares size and capacity and increments size when there is space:

```asm
; 2267911
+06 mov r10d, r8d
+0c mov r8d, dword ptr [rcx]
+12 cmp r8d, r10d
+15 jae +0x26
+17 lea edx, [r8 + 1]
+1e mov dword ptr [rcx], edx
```

When full, the helper dispatches via `+0x38 call qword ptr [r11]` (empty initial allocation) or `+0x61 call qword ptr [r11 + 8]` (growth). In table `1402049`, slot 0 is `2188663`, slot 1 is `2188699`; both use `+0 mov rcx, qword ptr [rcx + 8]` to recover the array from the adapter, and `+4 jmp` to `2267916` / `2267917`, respectively. `2267916` does `+0x44 imul edx, esi; +0x4d call 2267872; +0x52 mov qword ptr [rdi], rax; +0x5a mov dword ptr [rdi + 8], ebx` (allocates capacity × item size, records pointer and capacity). Growth in `2267917` reads the old capacity with `+0x11 mov eax, dword ptr [rcx + 8]; +0x1b add eax, eax`, computes bytes via `+0xbe imul edx, r14d; +0xc8 call 2267872`, then stores pointer/capacity with `+0x152 mov qword ptr [rbx], rbp; +0x155 mov dword ptr [rbx + 8], esi`.

**7 — read/write lock.** `2194167 +0x14 lea rbx, [rcx + 0x78]; +0x1b mov rcx, rbx; +0x25 call 2267897` acquires a read lock at list `+0x78`; the list constructor `2194153` also takes the write lock at the same offset with `+0x77 lea rbx, [rsi + 0x78]; +0x8d mov rcx, rbx; +0x90 call 2267898` before populating from the container.

**7 — item/Stack member identity and ownership.** In `2194062`, `+0x16 mov qword ptr [rcx], rdx` initializes the item object at `+0`; `+0x2a mov rbx, rcx` saves that item pointer. `+0x60 lea rcx, [rip + disp32]` resolves to the RTTI-identified Stack vtable `938404`; the following construction writes the new Stack's vptr at `+0`, null next at `+0x10`, extra at `+0x18`, count at `+0x20`, then retains and installs the Stack pointer in the item at `+8`:

```asm
+67 mov qword ptr [rax], rcx
+6a mov qword ptr [rax + 0x10], rbp
+6e mov qword ptr [rax + 0x18], rdi
+77 lock inc dword ptr [rdi]
+7a mov dword ptr [rax + 0x20], esi
+94 lock inc dword ptr [rax + 8]
+98 mov qword ptr [rbx + 8], rax
```

`2194071` starts at `item.stackData` (`+0 mov rcx, qword ptr [rcx + 8]`), sums counts (`+0xb add eax, dword ptr [rcx + 0x20]`), follows next (`+0xe mov rcx, qword ptr [rcx + 0x10]`) until null. `2194079` selects a Stack by index (`+9 mov rcx, qword ptr [rcx + 8]`; `+0x1a mov rcx, qword ptr [rcx + 0x10]`) then uses that Stack's ExtraDataList (`+0x30 mov rcx, qword ptr [rcx + 0x18]; +0x37 call 2190164`), while loading the item object separately (`+0x34 mov rbx, qword ptr [rbx]`). `2190164 +0 mov dl, 0x99; +2 jmp 2190936` confirms this is the extra-list receiver, not the text extra itself. Stack vtable slot 0 (`2194138`) releases these pointers: `+0x14 mov rsi, qword ptr [rcx + 0x18]; +0x33 lock xadd dword ptr [rsi], eax`, then `+0x4f mov rcx, qword ptr [rbx + 0x10]; +0x58 lock xadd dword ptr [rcx + 8], edi`, with each subtracting one and destroying the child if the prior count was one. The construction/retain and destructor/release pairs distinguish intrusive smart pointers from borrowed raw pointers.

### Cell, DataHandler and WorldSpace ID/RVA key

Instruction listings here use .191 RVAs. To locate the same instruction in .221 or .240, add that function's RVA delta from the table; the checker decodes at those sites independently. `call ID` and `RIP→ID` refer to resolved targets.

| Address Library ID (role inferred below) | 191 RVA | 221 RVA | 240 RVA |
|---|---|---|---|
| 2200159 (CELL constructor) | `0x4C1E30` | `0x4C1E40` | `0x4C2160` |
| 2200618 (CELL attach reference) | `0x4E87C0` | `0x4E87D0` | `0x4E8AF0` |
| 2200665 (append ref pointer) | `0x4ED600` | `0x4ED610` | `0x4ED930` |
| 2267911 (append/index/size helper) | `0x1659520` | `0x1659620` | `0x1659940` |
| 2267912 (indexed insert helper) | `0x16595B0` | `0x16596B0` | `0x16599D0` |
| 2267915 (array storage allocator constructor) | `0x16597B0` | `0x16598B0` | `0x1659BD0` |
| 2720923 (initialize size dword) | `0x1659470` | `0x1659570` | `0x1659890` |
| 2200378 (iterate CELL refs under lock) | `0x4D4930` | `0x4D4940` | `0x4D4C60` |
| 2192245 (reentrant spin lock) | `0x2DD4E0` | `0x2DD4E0` | `0x2DD800` |
| 2200453 (CELL-specific spin-lock acquisition) | `0x4DBC60` | `0x4DBC70` | `0x4DBF90` |
| 2192266 (data-handler constructor) | `0x2DE970` | `0x2DE970` | `0x2DEC90` |
| 2192271 (add form to handler) | `0x2DF1C0` | `0x2DF1C0` | `0x2DF4E0` |
| 2192272 (remove form from handler) | `0x2DF5F0` | `0x2DF5F0` | `0x2DF910` |
| 2192482 (Ni array indexed insert) | `0x2F4350` | `0x2F4350` | `0x2F4670` |
| 2192483 (Ni array resize storage) | `0x2F43E0` | `0x2F43E0` | `0x2F4700` |
| 2192290 (indexed interior-cell getter) | `0x2E3750` | `0x2E3750` | `0x2E3A70` |
| 2192434 (compact Ni array) | `0x2F1DE0` | `0x2F1DE0` | `0x2F2100` |
| 2202837 (WRLD constructor) | `0x5706C0` | `0x5706D0` | `0x5709F0` |
| 2202891 (insert exterior CELL into WRLD) | `0x574A30` | `0x574A40` | `0x574D60` |
| 2200215 (exterior cell grid coordinates) | `0x4C6AE0` | `0x4C6AF0` | `0x4C6E10` |
| 2202952 (cell-map insertion) | `0x57A500` | `0x57A510` | `0x57A830` |
| 2202843 (iterate and clone WRLD cells) | `0x5713E0` | `0x5713F0` | `0x571710` |
| 2200177 (CELL persistent flag test) | `0x4C4B10` | `0x4C4B20` | `0x4C4E40` |
| 2200346 (assign CELL back-pointer to WRLD) | `0x4D1D40` | `0x4D1D50` | `0x4D2070` |
| 2267872 (allocate table storage) | `0x16579C0` | `0x1657AC0` | `0x1657DE0` |
| 4818455 (RTTI dynamic cast) | `0x22BCB9E` | `0x22BCC7E` | `0x22C46AE` |

RTTI vtable witnesses: CELL ID 1515855 at `0x249B478 / 0x249B478 / 0x24A34D8` (`.?AVTESObjectCELL@@`); WRLD ID 227184 at `0x249F010 / 0x249F010 / 0x24A7070` (`.?AVTESWorldSpace@@`); interior-array ID 1485762 at `0x246C4C8 / 0x246C4C8 / 0x2474528` (`.?AV?$NiTPrimitiveArray@PEAVTESObjectCELL@@@@`). The cell-map constructor's chain sentinel ID 2661784 is `0x2ED9C7C / 0x2ED9C7C / 0x2EE4C7C`. The clone casts use CELL type descriptor ID 4839496 at `0x2F904B8 / 0x2F904F8 / 0x2F9B548` and TESForm descriptor ID 4839494 at `0x2F90498 / 0x2F904D8 / 0x2F9B528`.

### 8. CELL references and spin lock

Constructor 2200159 installs the CELL vtable and constructs an array starting at `+0x70` with its separate size dword at `+0x80`:

```asm
4C1E5D  lea rax, [RIP→ID 1515855]
4C1E64  mov qword ptr [rdi], rax
4C1ECE  lea rcx, [rdi + 0x70]
4C1ED2  call ID 2267915
4C1ED7  lea rcx, [rdi + 0x80]
4C1EDE  call ID 2720923
4C1F11  mov dword ptr [rdi + 0xc0], esi
4C1F17  mov dword ptr [rdi + 0xc4], esi
4C1F45  mov byte ptr [rdi + 0x1a], 0x3f
```

2200618 installs a reference after verifying it has a base object at `+0xE0`. The append specialization gets the array, reads its capacity (exclusive bound for allocation-free insertion in 2267911), passes the size by address, writes the pointer and takes an intrusive reference. The slot is a single qword, not a by-value REFR or a handle:

```asm
4E8817  cmp qword ptr [rdx + 0xe0], 0
4E8831  call ID 2200453
4E8836  lea rcx, [r14 + 0x70]
4E883A  mov rdx, r13
4E883D  call ID 2200665

4ED61C  add rcx, 0x10
4ED625  mov r9d, 8
4ED630  mov r8d, dword ptr [rdi + 8]
4ED634  call ID 2267911
4ED63E  mov rcx, qword ptr [rdi]
4ED643  mov qword ptr [rcx + rdx*8], rbx
4ED64C  lock inc dword ptr [rbx + 0x28]

1659526 mov r10d, r8d
165952C mov r8d, dword ptr [rcx]
1659532 cmp r8d, r10d
1659535 jae 0x1659546                 ; grow path (RVA)
1659537 lea edx, [r8 + 1]
165953E mov dword ptr [rcx], edx
```

2200378 locks the exact `+0xC0` address (`edx = 0` for the optional lock tag), walks from `size-1` to zero and temporarily acquires each REFR. On the last lock release it clears owner and CASes the recursion count from 1 to 0; otherwise it atomically decrements the recursion count:

```asm
4D49FE  lea rbx, [r13 + 0xc0]
4D4A05  xor edx, edx
4D4A07  mov rcx, rbx
4D4A0A  call ID 2192245
4D4A0F  mov eax, dword ptr [r13 + 0x80]
4D4A16  sub eax, 1
4D4A19  movsxd r14, eax
4D4A20  mov rax, qword ptr [r13 + 0x70]
4D4A24  mov rsi, qword ptr [rax + r14*8]
4D4A2D  lock inc dword ptr [rsi + 0x28]
4D4A50  mov rax, qword ptr [rsi + 0xe0]
...
4D4A94  mov eax, dword ptr [rbx + 4]
4D4A97  lea ecx, [rax - 1]
4D4A9A  test ecx, ecx
4D4A9C  jne 0x4D4AAF                  ; RVA
4D4A9E  xor r14d, r14d
4D4AA1  mov dword ptr [rbx], r14d
4D4AA4  mfence
4D4AA7  lock cmpxchg dword ptr [rbx + 4], r14d
4D4AAF  lock dec dword ptr [rbx + 4]
```

2192245 verifies the acquire layout directly: `2DD4F5 cmp dword ptr [rdi], eax`, `2DD4F9 lock inc dword ptr [rdi + 4]` for the current owner; `2DD514 xor eax, eax`, `2DD516 mov ebp, 1`, `2DD51B lock cmpxchg dword ptr [rdi + 4], ebp`, and `2DD541 mov dword ptr [rdi], esi` on first acquisition. In the fork `BSLock.h`, `lock` wraps relocation pair `(1425657, 2192245)` (OG, NG/AE), whereas `unlock` is inline atomic operations and needs no AE ID. The executable's release sequence has the explicit `mfence` shown above.

### 9. Data handler form arrays and interior cells

2192266 starts a loop at `+0x68`, calling the same array constructors as CELL, moving `0x18` bytes each iteration for `0x9F` entries; this ends at `0xF50`. In 2192271 the generic route inserts the input form using `formType*24 + 0x68`, with `edx` originally loaded from the form's byte at `+0x1A`. Capacity is at relative `+8`, size at `+0x10`, and the resulting index is applied to the base as an eight-byte pointer slot.

```asm
2DE9D3  lea rbx, [rdi + 0x68]
2DE9D9  mov esi, 0x9f
2DE9F0  mov rcx, rbx
2DE9F3  call ID 2267915
2DE9F8  lea rcx, [rbx + 0x10]
2DE9FC  call ID 2720923
2DEA01  add rbx, 0x18
2DEA05  sub rsi, 1
2DEA09  jne 0x2DE9F0                  ; RVA

2DF20E  movzx edx, byte ptr [rdx + 0x1a]
...
2DF50A  movzx eax, dl
2DF518  lea rcx, [rax + rax*2]
2DF51C  lea rsi, [rsi + rcx*8]
2DF520  add rsi, 0x68
2DF535  mov r8d, dword ptr [rsi + 8]
2DF539  lea rcx, [rsi + 0x10]
2DF53D  call ID 2267911
2DF549  mov rax, qword ptr [rsi]
2DF54C  mov qword ptr [rax + rcx*8], rdi
```

2202837 loads the primary WRLD vtable (`5706FB lea rax, [RIP→ID 227184]`, `570707 mov qword ptr [rsi], rax`) and writes `5708FF mov byte ptr [rsi + 0x1a], 0x4a`. Dispatch in 2192271: `2DF21D lea eax, [rdx - 0x15]`; `2DF220 cmp eax, 0x4d`; `2DF229 lea r8, [rip - 0x2df230]` sets image base; `2DF230 movzx eax, byte ptr [r8 + rax + 0x2df594]` remaps the selector; `2DF239 mov ecx, dword ptr [r8 + rax*4 + 0x2df570]`; `2DF241 add rcx, r8`; `2DF244 jmp rcx`. On 240, the base instruction computes the same image base with `[rip - 0x2DF550]`, remap base `0x2DF8B4`, jump table base `0x2DF890`. Decoding each image's two tables gives WRLD `0x4A` → remap `4` → `0x2DF2FB / 0x2DF2FB / 0x2DF61B`, CELL `0x3F` → remap `2` → `0x2DF246 / 0x2DF246 / 0x2DF566`. The WRLD specialized route inserts at the front of the slot at `0x68 + 0x4A*0x18 = 0x758`:

```asm
2DF2FB  add rsi, 0x758
2DF311  mov dword ptr [rsp + 0x28], 8
2DF328  mov r9d, dword ptr [rsi + 8]
2DF32C  lea rcx, [rsi + 0x10]
2DF330  mov r8, qword ptr [rsi]
2DF333  mov dword ptr [rsp + 0x20], 0
2DF33B  call ID 2267912
2DF348  mov rax, qword ptr [rsi]
2DF34B  mov qword ptr [rax], rdi
```

The same handler constructor installs the polymorphic Ni array vtable at `+0xF58` (RTTI is exactly `NiTPrimitiveArray<TESObjectCELL*>`), a null data pointer at `+0xF60`, and zeroes the packed uint16 size words before setting growth to 100. The dispatched CELL branch tests the interior bit, then appends the original `TESObjectCELL*` at the next high-water index (`+0xF6A`), unlike the WRLD branch:

```asm
2DEA0B  mov dword ptr [rdi + 0xf68], ebp     ; ebp = 0
2DEA11  lea rax, [RIP→ID 1485762]
2DEA18  mov qword ptr [rdi + 0xf58], rax
2DEA26  mov dword ptr [rdi + 0xf6c], 0x10000 ; size=0, initial growth=1
2DEA37  mov qword ptr [rdi + 0xf60], rbp
2DEB5D  mov eax, 0x64
2DEB62  mov word ptr [rdi + 0xf6e], ax

2DF24E  test byte ptr [rdi + 0x40], 1
2DF252  mov qword ptr [rsp + 0x68], rdi
2DF257  je 0x2DF550                         ; RVA
2DF25D  movzx edx, word ptr [rsi + 0xf6a]
2DF264  lea rcx, [rsi + 0xf58]
2DF26B  lea r8, [rsp + 0x68]
2DF270  call ID 2192482
```

In 2192482 the unsigned 16-bit capacity bounds the index; it expands by using index plus growth. The 16-bit high-water becomes `index+1` for append/extension, while the 16-bit occupied count is increased only if the new value is non-null. The actual store is `data[index]`. The resize helper records the 16-bit capacity and requests storage for that many eight-byte slots:

```asm
2F435F  movzx eax, word ptr [rcx + 0x10]
2F436B  cmp edx, eax
2F436D  jb 0x2F437A                         ; RVA
2F436F  movzx edx, word ptr [rcx + 0x16]
2F4373  add edx, edi
2F4375  call ID 2192483
2F437A  movzx eax, word ptr [rbx + 0x12]
2F437E  cmp edi, eax
2F4380  jb 0x2F439C                         ; RVA
2F4382  lea eax, [rdi + 1]
2F4385  mov word ptr [rbx + 0x12], ax
2F4389  cmp qword ptr [rsi], 0
2F438D  je 0x2F4393                         ; RVA
2F438F  inc word ptr [rbx + 0x14]
2F4393  mov rcx, qword ptr [rbx + 8]
2F43D3  mov qword ptr [rcx + rdx*8], rax

2F443A  mov word ptr [rbx + 0x10], dx
2F4443  movzx ecx, dx
2F4446  shl rcx, 3
2F4457  mov qword ptr [rbx + 8], rax
```

Removal 2192272 scans pointer slots in `+0xF60` to the 16-bit high-water `+0xF6A` (`2DF69F movzx r8d, word ptr [rdi + 0xf6a]`, `2DF6B2 mov rdx, qword ptr [rdi + 0xf60]`, `2DF6C0 mov r9, qword ptr [rdx + rcx*8]`, `2DF6CE cmp ecx, r8d`). It decrements the occupied count for a non-null entry (`2DF6EE mov eax, 0xffff`; `2DF6F3 add word ptr [rdi + 0xf6c], ax`), clears the slot (`2DF701 mov qword ptr [rdx + r10*8], 0`) and calls 2192434 on `handler+0xF58`. The engine indexed getter 2192290 also compacts first: `2E375F add rcx, 0xf58`; `2E3766 call ID 2192434`; `2E376B mov rax, qword ptr [rdi + 0xf60]`; `2E3772 mov rax, qword ptr [rax + rbx*8]`. The compactor compares occupied count with high-water (`2F1DE6 movzx eax, word ptr [rcx + 0x14]`; `2F1DED movzx edx, word ptr [rcx + 0x12]`; `2F1DF1 cmp ax, dx`; `2F1DF4 je 0x2F1EAE`), skips null values (`2F1E27 mov rdx, qword ptr [r9 + rax*8]`; `2F1E2B test rdx, rdx`; `2F1E2E je 0x2F1E42`), compacts non-null values forward (`2F1E3A mov qword ptr [r9 + rax*8], rdx`), and writes the occupied count as the new high-water and capacity (`2F1E4B movzx eax, word ptr [rbx + 0x14]`; `2F1E53 mov word ptr [rbx + 0x12], ax`; `2F1E57 mov word ptr [rbx + 0x10], ax`). **A read-only walk should use the high-water span and skip nulls, or use the fork's hole-skipping iterators, unless the caller can establish that the array is dense.** This is a semantic caution, not an AE displacement mismatch. No claim is made that holes actually persist at the DSD call boundary.

### 10. WRLD exterior-cell map and persistent cell

2202837 constructs the table with `capacity`, `free` and `good` at `+0x4C/+0x50/+0x54`, sentinel `+0x58`, pointer `+0x68`. It allocates `capacity * 0x18` bytes and zeros every link at `entry+0x10`, leaving each slot unoccupied; initializes the separate pointer at `+0x70` to null.

```asm
570734  lea rax, [RIP→ID 2661784]
57073B  mov dword ptr [rsi + 0x4c], ebx
57073E  mov qword ptr [rsi + 0x58], rax
570742  mov dword ptr [rsi + 0x50], ebx
570745  mov dword ptr [rsi + 0x54], ebx
570773  lea rdx, [rax + rax*2]               ; rax = zero-extended ebx
570777  shl rdx, 3
57077B  call ID 2267872
570783  mov qword ptr [rsi + 0x68], rax
57078C  mov ecx, dword ptr [rsi + 0x4c]
570793  add rax, 0x10
5707A0  mov qword ptr [rax], r12             ; r12 = 0
5707A3  lea rax, [rax + 0x18]
5707A7  add ecx, -1
5707AA  jne 0x5707A0                        ; RVA
5707AC  mov qword ptr [rsi + 0x70], r12
```

Insertion 2202891 tests for an exterior cell (`test byte ptr [rdx+0x40],1`, then `jne` reject). 2200215 reads the exterior coordinate pair (`4C6AE6 mov rcx, qword ptr [rcx + 0x50]`; `4C6AEF mov eax, dword ptr [rcx]`; `4C6AF1 mov dword ptr [rdx], eax`; `4C6AF3 mov eax, dword ptr [rcx + 4]`; `4C6AF6 mov dword ptr [r8], eax`). The ordinary exterior branch forms a 32-bit packed key from the two low 16-bit values and passes the original cell pointer by address:

```asm
574B23  mov rcx, qword ptr [rsp + 0x78]      ; original cell arg
574B38  call ID 2200215
574B3D  movsx ecx, word ptr [rsp + 0x80]
574B4A  movzx eax, word ptr [rsp + 0x88]
574B52  lea r8, [rsp + 0x78]
574B57  shl ecx, 0x10
574B5A  lea rdx, [rsp + 0x20]
574B5F  or ecx, eax
574B61  mov dword ptr [rsp + 0x20], ecx
574B65  lea rcx, [rbp + 0x48]                ; rbp = world
574B69  call ID 2202952
```

2202952 uses `rbx = world+0x48`, so its offsets `+4/+8/+0xC/+0x10/+0x20` are the world offsets `+0x4C/+0x50/+0x54/+0x58/+0x68`, or exactly fork map offsets `+0xC/+0x10/+0x14/+0x18/+0x28` relative to `world+0x40`. It masks the hash with `capacity-1`, uses a `0x18` stride and compares the 32-bit key at entry `+0`. Link `+0x10` is null for an empty slot and matches the per-map sentinel at end of an occupied chain. A collision starts looking for a free slot from `good`, wrapping with the capacity mask; a successful insertion decrements the free-slot count. The direction of this search differs from the fork's header (AE searches backward; fork forward) but the field and its use do not affect read-only iteration.

```asm
57A51C  mov r12, r8                          ; &cell
57A51F  mov r15, rdx                         ; &key
57A524  mov rbx, rcx                         ; world+0x48
57A540  mov r14, qword ptr [rbx + 0x20]
57A549  mov r9d, dword ptr [rbx + 4]
57A54D  dec r9d
57A553  and rcx, rbp
57A556  lea rax, [rcx + rcx*2]
57A55A  cmp qword ptr [r14 + rax*8 + 0x10], 0
57A570  cmp dword ptr [rax], ecx
57A578  mov rax, qword ptr [rax + 0x10]
57A57C  cmp rax, qword ptr [rbx + 0x10]
57A582  mov r8d, dword ptr [rbx + 8]
57A58D  mov edx, dword ptr [rbx + 0xc]
57A590  lea eax, [rdx - 1]
57A593  and eax, r9d
57A598  mov dword ptr [rbx + 0xc], edx
57A59F  cmp qword ptr [r14 + rax*8 + 0x10], 0
...
57A666  mov eax, dword ptr [r15]
57A669  mov rcx, qword ptr [rbx + 0x10]
57A66D  mov dword ptr [rsi], eax
57A66F  mov rax, qword ptr [r12]
57A673  mov qword ptr [rsi + 8], rax
57A677  mov qword ptr [rsi + 0x10], rcx
57A67B  dec dword ptr [rbx + 8]
```

2202843 iterates like the fork, independently of insertion: `end = entries + capacity*0x18`, skip precisely entries with `next == nullptr`, read the cell at `entry+8`. It clones the value via its virtual method and casts the result to `TESObjectCELL*` using the RTTI descriptor:

```asm
5714CC  mov r8, qword ptr [r13 + 0x68]       ; r13 = world
5714D5  mov eax, dword ptr [r13 + 0x4c]
5714E4  lea rcx, [rax + rax*2]
5714E8  lea rsi, [r8 + rcx*8]
5714F0  cmp rdi, rsi
5714F3  jae 0x57150F                        ; RVA
5714F5  mov rcx, qword ptr [rdi + 0x10]
5714F9  lea rax, [rdi + 0x18]
5714FD  test rcx, rcx
571507  cmove rdi, rax
57150D  je 0x5714F0                         ; RVA
57152A  mov rcx, qword ptr [rdi + 8]
57153B  call qword ptr [rax + 0x60]
57153E  lea r9, [RIP→ID 4839496]
571550  lea r8, [RIP→ID 4839494]
571559  call ID 4818455
...
571580  add rdi, 0x18
571584  cmp rdi, rsi
571587  jae 0x571590                        ; RVA
571589  cmp qword ptr [rdi + 0x10], 0
57158E  je 0x571580                         ; RVA
```

The other branch in 2202891 places the CELL at `world+0x70` if the CELL's base form flags include bit 10 (2200177), rather than passing it to the map insert. It links the cell back to the world at cell `+0xC8` (2200346). The clone/iteration routine also treats `world+0x70` separately from the map and casts the cloned result with the same CELL RTTI:

```asm
574A7F  call ID 2200177
574A84  test al, al
574A86  je 0x574AB1                         ; ordinary-cell branch RVA
574A88  cmp qword ptr [rbp + 0x70], 0
574A93  mov rdx, rbp
574A96  mov qword ptr [rbp + 0x70], rbx
574A9A  mov rcx, rbx
574A9D  call ID 2200346

4C4B10  mov eax, dword ptr [rcx + 0x10]
4C4B13  shr eax, 0xa
4C4B16  and al, 1
4D1D46  mov qword ptr [rcx + 0xc8], rdx

57159A  mov rcx, qword ptr [r13 + 0x70]
5715AB  call qword ptr [rax + 0x60]
5715AE  lea r9, [RIP→ID 4839496]
5715C0  lea r8, [RIP→ID 4839494]
5715C9  call ID 4818455
5715D3  mov qword ptr [rbp + 0x70], rax
```

### 11. Read-lock release

`2194167` passes the same lock address saved in `rbx` to both calls (`+0x1b mov rcx, rbx; +0x25 call 2267897`, then `+0x5f mov rcx, rbx; +0x62 call 2267903`). Similarly `FindExtra` (`2190936`) loads `[extraList+0x20]`'s address with `+0x14 lea rbx, [rcx + 0x20]`, then `+0x1b mov rcx, rbx; +0x21 call 2267897` and `+0x85 mov rcx, rbx; +0x88 call 2267903`. The acquire uses `2267897 +0x06 mov rdi, rcx; +0x16 lock inc dword ptr [rdi + 4]` or `+0x2f lock cmpxchg dword ptr [rdi + 4], ecx`. The release leaf is `2267903 +0 lock dec dword ptr [rcx + 4]; +4 ret` (`f0 ff 49 04 c3`). The AE call is not inline at these sites; the fork's inline operation is nevertheless equivalent. No change needed for these facts.

### 12. Debug-path relocation audit

`TESForms.h`: `GetFormTypeString()` indexes the table supplied by `GetFormEnumString()`; the only relocation on that path is data ID **1309967**. `TESDataHandler.h`: `LookupForm()` is header code, not an engine call. Its `TESForm::GetFormByID()` → `GetAllForms()` path uses map `RelocationID(422985, 2689178)` and lock `RelocationID(691815, 2689189)` in `TESForms.h`. The lock wrappers are separate from those two data relocations; `lock_read` 2267897 is already audited in Phase C. The type-safe `LookupForm<T>()` wrapper adds a form-type check and `As<T>()`, not a further lookup-function ID.

| Lookup | .137 | .159 | .169 | .191 | .221 | .240 |
|---|---:|---:|---:|---:|---:|---:|
| 1309967 | `0x2EBFB60` | `0x2EC0B60` | `0x2EC5B60` | `0x2ED0B60` | `0x2ED0B60` | `0x2EDBB60` |
| 422985 / 2689178 (OG / second-slot map IDs) | absent | absent | absent | absent | absent | absent |
| 691815 / 2689189 (OG / second-slot lock IDs) | absent | absent | absent | absent | absent | absent |

This is a bin-membership audit only, as requested; no additional proof of the debug functions or their layouts is inferred from it.

### Cross-check: the fork's inline `ExtraDataList::GetByType`

AE FindExtra **2190936** (`0x2A0030 / 0x2A0030 / 0x2A0350`) acquires the read lock at `this+0x20` (`+0x14 lea rbx,[rcx+0x20]`, `+0x1B mov rcx,rbx`, `+0x21 call 2267897`). Its bitmap pointer is `+0x26 mov r8,[rbp+0x18]`, then `+0x3B shr eax,3`, `+0x43 and ecx,7`, `+0x4B shl edx,cl`, `+0x4D test [rax+r8],dl`. It loads `head` at `+0x53 mov rcx,[rbp+8]`; the walk compares the byte type (`+0x68 cmp [rcx+0x12],sil`) and follows `+0x70 mov rcx,[rcx+8]`. At `+0x85 mov rcx,rbx`, `+0x88 call 2267903`, it releases the same lock. This matches the fork's inline lookup fields for the three requested type IDs.

## Re-check output

Command: `python -I phaseE/prove_namerefresh.py`. The checker uses `extras.py`, `inventory.py`, and `world.py` beside it and the existing `ae/lib.py`. PASS means the documented evidence was reproduced, not necessarily that AE agrees with the fork. Unproven subclaims are not counted as a pass.

```text
1.11.191 PE file version: PASS
1.11.191 1 entry (0x40 bytes): PASS
1.11.191 1 ABI and void: PASS
1.11.191 1 0x99 preserve, unlink, add, update: PASS
1.11.191 2 type 0x99: PASS
1.11.191 2 type 0x35: PASS
1.11.191 2 type 0xBA: PASS
1.11.191 3 fixed string +0x18: PASS
1.11.191 3 message +0x20 and quest +0x28: PASS
1.11.191 3 flags +0x10 bit 0: PASS
1.11.191 4 base +0x18 smart pointer +0x20: PASS
1.11.191 5 values +0x18 and index block/records: PASS
1.11.191 3 case-sensitive pool comparison: PASS
1.11.191 6 REFR inventoryList: PASS
1.11.191 7 list array data/size, item stride/object: PASS
1.11.191 7 list array capacity: PASS
1.11.191 7 list read/write lock: PASS
1.11.191 7 item stackData and Stack next/extra/count: PASS
1.11.191 11 read lock release: PASS
1.11.191 8.references: PASS
1.11.191 8.spinLock: PASS
1.11.191 9.formArrays/WRLD: PASS
1.11.191 9.interiorCells: PASS
1.11.191 10.cellMap: PASS
1.11.191 10.persistentCell: PASS
1.11.191 12 Address Library IDs: PASS
1.11.191 Bonus FindExtra layout: PASS
1.11.221 PE file version: PASS
1.11.221 1 entry (0x40 bytes): PASS
1.11.221 1 ABI and void: PASS
1.11.221 1 0x99 preserve, unlink, add, update: PASS
1.11.221 2 type 0x99: PASS
1.11.221 2 type 0x35: PASS
1.11.221 2 type 0xBA: PASS
1.11.221 3 fixed string +0x18: PASS
1.11.221 3 message +0x20 and quest +0x28: PASS
1.11.221 3 flags +0x10 bit 0: PASS
1.11.221 4 base +0x18 smart pointer +0x20: PASS
1.11.221 5 values +0x18 and index block/records: PASS
1.11.221 3 case-sensitive pool comparison: PASS
1.11.221 6 REFR inventoryList: PASS
1.11.221 7 list array data/size, item stride/object: PASS
1.11.221 7 list array capacity: PASS
1.11.221 7 list read/write lock: PASS
1.11.221 7 item stackData and Stack next/extra/count: PASS
1.11.221 11 read lock release: PASS
1.11.221 8.references: PASS
1.11.221 8.spinLock: PASS
1.11.221 9.formArrays/WRLD: PASS
1.11.221 9.interiorCells: PASS
1.11.221 10.cellMap: PASS
1.11.221 10.persistentCell: PASS
1.11.221 12 Address Library IDs: PASS
1.11.221 Bonus FindExtra layout: PASS
1.11.240 PE file version: PASS
1.11.240 1 entry (0x40 bytes): PASS
1.11.240 1 ABI and void: PASS
1.11.240 1 0x99 preserve, unlink, add, update: PASS
1.11.240 2 type 0x99: PASS
1.11.240 2 type 0x35: PASS
1.11.240 2 type 0xBA: PASS
1.11.240 3 fixed string +0x18: PASS
1.11.240 3 message +0x20 and quest +0x28: PASS
1.11.240 3 flags +0x10 bit 0: PASS
1.11.240 4 base +0x18 smart pointer +0x20: PASS
1.11.240 5 values +0x18 and index block/records: PASS
1.11.240 3 case-sensitive pool comparison: PASS
1.11.240 6 REFR inventoryList: PASS
1.11.240 7 list array data/size, item stride/object: PASS
1.11.240 7 list array capacity: PASS
1.11.240 7 list read/write lock: PASS
1.11.240 7 item stackData and Stack next/extra/count: PASS
1.11.240 11 read lock release: PASS
1.11.240 8.references: PASS
1.11.240 8.spinLock: PASS
1.11.240 9.formArrays/WRLD: PASS
1.11.240 9.interiorCells: PASS
1.11.240 10.cellMap: PASS
1.11.240 10.persistentCell: PASS
1.11.240 12 Address Library IDs: PASS
1.11.240 Bonus FindExtra layout: PASS
1.11.137 12 Address Library IDs (bin only): PASS
1.11.159 12 Address Library IDs (bin only): PASS
1.11.169 12 Address Library IDs (bin only): PASS
Phase E: 84/84 checks passed (0 failed)
```
