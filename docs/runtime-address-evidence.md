# Runtime address evidence

## Method and limitations

Read-only offline Python/Capstone analysis. No game execution, installs, D: writes, production edits or commits. Inline Python, no scripts persisted. Capstone already installed. REA tools/CLI unavailable; this is not REA Evidence.

**ID existence is NOT cross-runtime semantic proof. All NG/AE identities, structure layouts and in-game results remain UNVERIFIED. No values guessed.** NG and AE .137/.159/.169 executables are unavailable. Disk images have .bind sections and potentially protected/encrypted contents; bytes, entropy and syntactic disassembly below must not be mistaken for loaded code. No unpacking attempted.

The dual build and the NG/AE disable path before hooks and listeners were not independently tested here.

Decoder provenance: `extern/CommonLibF4/AddressLibDecoder/src/main.cpp`: uint64 count then count uint64 ID/RVA pairs. Bundled decoder was NOT executed because it writes sibling .txt files. Inline equivalent checked exact length (8+16*count) and ID uniqueness.

## Explicit source inventory

Project numeric REL IDs, hook arguments, direct RE vtable references; not every transitive CommonLib ID. Line references are snapshot values.

```text
src/ActivateText.cpp:16: // Hook method: both are prologue hooks (HookUtil::InstallPrologueHook), the same method FallHook uses on
src/ActivateText.cpp:32: constexpr std::size_t kActionSlot = 0x08;
src/ActivateText.cpp:112: HookUtil::InstallPrologueHook<GetActivateText>(1077221, "TESObjectREFR::GetActivateText");
src/ActivateText.cpp:113: HookUtil::InstallPrologueHook<ShowRollover>(414044, "HUDRollover::ShowRollover");
src/ConstTranslation.cpp:156: static_assert(sizeof(QuestObjective) == 0x28);
src/ConstTranslation.cpp:428: constexpr std::uint64_t         copyComponentID = 1510422;
src/ConstTranslation.cpp:429: REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::TESFullName[0] };
src/ConstTranslation.cpp:430: REL::Relocation<std::uintptr_t> target{ REL::ID(copyComponentID) };
src/ConstTranslation.cpp:433: logger::error("Hook TESFullName::CopyComponent not installed: ID {} is {:X}, vtable slot 6 is {:X}", copyComponentID, target.address(), slot);
src/ConstTranslation.cpp:436: HookUtil::InstallPrologueHook<FullNameCopyComponent>(copyComponentID, "TESFullName::CopyComponent");
src/Dialogue.cpp:84: static constexpr std::array<std::uint8_t, 8> pattern{ 0x48, 0x83, 0xC6, 0x18, 0x48, 0x83, 0xC7, 0x18 };
src/Dialogue.cpp:95: const auto     instruction = match + 0xC;
src/Dialogue.cpp:100: return reinterpret_cast<PromptMap*>(operand - 0x28);
src/Dialogue.cpp:116: HookUtil::InstallPrologueHook<DialogueResponseCtor>(755245, "DialogueResponse::DialogueResponse");
src/DialogueOptions.cpp:34: // Hook method: prologue hooks (HookUtil::InstallPrologueHook), as FallHook does for the same IDs. A prologue the
src/DialogueOptions.cpp:263: static_assert(sizeof(ValueLayout) == sizeof(Value));
src/DialogueOptions.cpp:273: static REL::Relocation<bool(ObjectInterface*, void*, std::uint32_t, Value*)> func{ REL::ID(827659) };
src/DialogueOptions.cpp:588: const auto context = HookUtil::InstallPrologueHook<GetCurrentTopicInfo>(781358, "BGSSceneActionPlayerDialogue::GetCurrentTopicInfo");
src/DialogueOptions.cpp:589: const auto button = HookUtil::InstallPrologueHook<SetButtonText>(643840, "DialogueMenuUtils::SetButtonText");
src/DialogueOptions.cpp:594: if (!HookUtil::InstallPrologueHook<SetMemberHook>(1360149, "Scaleform::GFx::Value::ObjectInterface::SetMember")) {
src/LeveledListNames.cpp:16: constexpr std::ptrdiff_t kLeveledListOffset = 0x68;
src/LeveledListNames.cpp:17: constexpr std::size_t    kGetOverrideNameSlot = 0x0A;
src/LeveledListNames.cpp:93: REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE::TESLevItem[1] };
src/MiscTypes.cpp:81: constexpr std::ptrdiff_t kOffsetOG = 0x160;
src/NameRefresh.cpp:303: const auto  address = REL::ID(457340).address();
src/PortedConstFields.cpp:11: [[nodiscard]] constexpr Slot Written(bool a_changed) noexcept { return a_changed ? Slot::kApplied : Slot::kUnchanged; }
src/PortedConstFields.cpp:99: constexpr std::uint64_t kFindRegionDataID = 657534;
src/PortedConstFields.cpp:104: constexpr std::ptrdiff_t kMapNameOffset = 0x10;
src/PortedConstFields.cpp:119: static REL::Relocation<FindRegionData_t> find{ REL::ID(kFindRegionDataID) };
src/PortedConstFields.cpp:140: constexpr std::ptrdiff_t kFunctionDataOffset = 0x18;
src/PortedConstFields.cpp:141: constexpr std::ptrdiff_t kChoiceLabelOffset = 0x18;
src/PortedConstFields.cpp:142: constexpr std::ptrdiff_t kChoiceIndexOffset = 0x28;
src/PortedConstFields.cpp:152: static const REL::Relocation<std::uintptr_t> entryVtable{ RE::VTABLE::BGSEntryPointPerkEntry[0] };
src/PortedConstFields.cpp:153: static const REL::Relocation<std::uintptr_t> choiceVtable{ RE::VTABLE::BGSEntryPointFunctionDataActivateChoice[0] };
src/RuntimeText.cpp:158: HookUtil::InstallPrologueHook<GetDescription>(523613, "TESDescription::GetDescription");
src/RuntimeText.cpp:159: HookUtil::InstallPrologueHook<GetJournalText>(139753, "TESQuest::GetJournalTextForStageItem");
```

Vtable constants in `extern/CommonLibF4/CommonLibF4/include/RE/VTABLE_IDs.h`: TESFullName 487988 (line 8029); TESLevItem [718716,1381610] (8042); BGSEntryPointPerkEntry 846981 (1912); BGSEntryPointFunctionDataActivateChoice 859152 (1905). Primary TESLevItem included as context.

## Address Library identities

Directory: Address Library for F4SE Plugins (`Data\F4SE\Plugins`)

| Version | Count | Size | SHA-256 |
|---|---:|---:|---|
| 1-10-163-0 | 1582975 | 25327608 | `d849ae8989c54a2ceebbc587c2a77f62a44f74d0c42a42e0bed30d65118d3395` |
| 1-10-980-0 | 594282 | 9508520 | `afd7f26dae4b682121bd9564296d10d15070f1ea7f81c6e6161ad29377ee9152` |
| 1-10-984-0 | 594329 | 9509272 | `c13545f2dab806b7d19c5aa83a0d7524337b5afc89af79460f9915020e049d75` |
| 1-11-137-0 | 650589 | 10409432 | `7b6f1387ea916c8017257a14dd02965c31a562bfc6481e8ee17ab5b2d1df1f4b` |
| 1-11-159-0 | 650669 | 10410712 | `04f3b00bb3f3bcbb31dac3b893fe4575e7d74727f855f31a0c8c3efab2b3d16b` |
| 1-11-169-0 | 650800 | 10412808 | `6e139724ab74191d0514cb01923a7de24c109bb4fe29b5571278ec08e81214c3` |
| 1-11-191-0 | 651571 | 10425144 | `3ae85753d5c42886ff34492dda4ea218726cfa2870c49f028d5c5727c9681f52` |
| 1-11-221-0 | 651581 | 10425304 | `2fdcc2a0926659c37255d2eaf335775240ec7fefdf6cb3b35b063faca25a448f` |
| 1-11-240-0 | 652306 | 10436904 | `65985cc2259384a13cffb538d74776e422e62b0cd3766485a000620242b72b06` |

## Every ID / version RVA

MISSING means exact numeric ID absent. Present does not imply semantic equivalence.

| ID | Role | 1-10-163-0 | 1-10-980-0 | 1-10-984-0 | 1-11-137-0 | 1-11-159-0 | 1-11-169-0 | 1-11-191-0 | 1-11-221-0 | 1-11-240-0 |
|---:|---|---|---|---|---|---|---|---|---|---|
| 1077221 | GetActivateText | 0x40BB30 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 414044 | ShowRollover | 0xAB19D0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 1510422 | TESFullName CopyComponent | 0x159780 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 755245 | DialogueResponse ctor | 0xCA1BF0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 781358 | GetCurrentTopicInfo | 0x2C6170 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 643840 | SetButtonText | 0x12B9FE0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 1360149 | GFx SetMember | 0x20D05E0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 827659 | GFx GetElement | 0x20A5B50 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 457340 | NameRefresh target | 0x89DC0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 657534 | FindRegionData | 0x28DA80 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 523613 | GetDescription | 0x14FAF0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 139753 | GetJournalTextForStageItem | 0x5DD850 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 487988 | VTABLE TESFullName[0] | 0x2C4C1B8 | 0x2276C10 | 0x2276C10 | 0x245A5D0 | 0x245B5D0 | 0x245D5D0 | 0x24625E0 | 0x24625E0 | 0x246A640 |
| 1381610 | VTABLE TESLevItem[1] | 0x2C788C8 | 0x22A19D0 | 0x22A19D0 | 0x2485340 | 0x2486340 | 0x2488330 | 0x248D340 | 0x248D340 | 0x24953A0 |
| 718716 | VTABLE TESLevItem[0] context | 0x2C78588 | 0x22A1690 | 0x22A1690 | 0x2485000 | 0x2486000 | 0x2487FF0 | 0x248D000 | 0x248D000 | 0x2495060 |
| 846981 | VTABLE BGSEntryPointPerkEntry[0] | 0x2CB6258 | 0x22DEB08 | 0x22DEB08 | 0x24C23E8 | 0x24C33E8 | 0x24C53D8 | 0x24CA3D8 | 0x24CA3E8 | 0x24D2448 |
| 859152 | VTABLE ActivateChoice[0] | 0x2CB4498 | 0x22DCDB8 | 0x22DCDB8 | 0x24C0698 | 0x24C1698 | 0x24C3688 | 0x24C8688 | 0x24C8698 | 0x24D06F8 |

## PE bytes and syntactic disassembly

Instruction addresses are RVAs. First 48 bytes at each present ID. No listing certifies a safe prologue or semantic identity.

### 1-10-163-0: `Fallout4.exe`

Size 65503104; SHA-256 `55f57947db9e05575122fae1088f0b0247442f11e566b56036caa0ac93329c36`; ImageBase `0x140000000`.

| Section | RVA | Virtual size | File offset | Raw size | Flags | Entropy bits/byte |
|---|---:|---:|---:|---:|---:|---:|
| .text | 0x1000 | 0x2C0AF8F | 0x400 | 0x2C0B000 | 0x60000020 | 8.00000 |
| .interpr | 0x2C0C000 | 0xA6DC | 0x2C0B400 | 0xA800 | 0x60000020 | 6.42241 |
| .rdata | 0x2C17000 | 0xAB3E30 | 0x2C15C00 | 0xAB4000 | 0x40000040 | 5.16939 |
| .data | 0x36CB000 | 0x306B318 | 0x36C9C00 | 0x201A00 | 0xC0000040 | 4.88535 |
| .pdata | 0x6737000 | 0x354C60 | 0x38CB600 | 0x354E00 | 0x40000040 | 7.13292 |
| .tls | 0x6A8C000 | 0x3571 | 0x3C20400 | 0x3600 | 0xC0000040 | 0.02310 |
| .rsrc | 0x6A90000 | 0xA67A0 | 0x3C23A00 | 0xA6800 | 0x40000040 | 5.10137 |
| .reloc | 0x6B37000 | 0x181126 | 0x3CCA200 | 0x181200 | 0x42000040 | 2.69974 |
| .bind | 0x6CB9000 | 0x2CB80 | 0x3E4B400 | 0x2CB80 | 0x60000000 | 7.94816 |

#### ID 1077221 at `0x40BB30` (.text)

Bytes: `e1 f9 ca 70 90 3e 44 20 86 7f a5 a5 21 38 a0 b9 5c 95 59 c4 00 b6 9e 8c 8b 04 70 1a 9d 64 4f 49 1b 4f ed 06 89 5e ba fc c8 53 4f 31 5a 76 44 fc`

Syntactic decode only:
```asm
0040BB30  e1 f9                            loope 0x40bb2b
0040BB32  ca 70 90                         retf 0x9070
0040BB35  3e 44 20 86 7f a5 a5 21          and byte ptr ds:[rsi + 0x21a5a57f], r8b
0040BB3D  38 a0 b9 5c 95 59                cmp byte ptr [rax + 0x59955cb9], ah
```

#### ID 414044 at `0xAB19D0` (.text)

Bytes: `25 99 0b 2f 22 9c d7 cd f0 49 a3 16 83 53 9b f0 7b 00 22 86 08 95 85 f1 c3 4a ae 35 65 5e 3c 3d 0f 71 aa c8 f8 35 7a 30 4f 05 81 67 ff 69 d7 23`

Syntactic decode only:
```asm
00AB19D0  25 99 0b 2f 22                   and eax, 0x222f0b99
00AB19D5  9c                               pushfq 
00AB19D6  d7                               xlatb 
00AB19D7  cd f0                            int 0xf0
00AB19D9  49 a3 16 83 53 9b f0 7b 00 22    movabs qword ptr [0x22007bf09b538316], rax
00AB19E3  86 08                            xchg byte ptr [rax], cl
00AB19E5  95                               xchg ebp, eax
00AB19E6  85 f1                            test ecx, esi
00AB19E8  c3                               ret 
00AB19E9  4a ae                            scasb al, byte ptr [rdi]
00AB19EB  35 65 5e 3c 3d                   xor eax, 0x3d3c5e65
```

#### ID 1510422 at `0x159780` (.text)

Bytes: `4a 2f d4 30 53 9a d1 38 0c d7 d1 36 05 69 b4 1b f3 32 fd ef 5b e6 ad 7e 22 4c 78 f4 2b b4 aa 2f cd 21 51 db 73 66 9a 7d 80 3f 3a 9b 06 21 c1 53`

Syntactic decode only:
```asm
(no decode)
```

#### ID 755245 at `0xCA1BF0` (.text)

Bytes: `43 f3 e5 02 c6 f6 46 92 ca 9b 1c 39 48 47 b8 e4 7f 95 f1 57 cc 52 3f d8 8a bc f4 dc 51 30 16 51 1e 8a 08 15 bb e6 56 38 d4 9a 70 cd c1 c3 57 95`

Syntactic decode only:
```asm
00CA1BF0  43 f3 e5 02                      in eax, 2
```

#### ID 781358 at `0x2C6170` (.text)

Bytes: `0c e9 0a c9 ca f4 a7 47 b2 ad 5b f0 79 a6 6e 41 a1 05 18 bc 95 0a 5e bc 74 3b 83 aa 28 6f 88 ec 41 5c 5f 05 b8 99 2a 4d d7 2c f6 8d 1e 79 6e 4d`

Syntactic decode only:
```asm
002C6170  0c e9                            or al, 0xe9
002C6172  0a c9                            or cl, cl
002C6174  ca f4 a7                         retf 0xa7f4
002C6177  47 b2 ad                         mov r10b, 0xad
002C617A  5b                               pop rbx
```

#### ID 643840 at `0x12B9FE0` (.text)

Bytes: `ed 49 72 65 8c cb bc 8c c9 7b f4 26 c3 c2 02 9c b3 71 18 fb f9 a9 b3 02 c5 9b 4a 7b d8 e3 25 2a 3f 17 87 c1 28 17 94 78 77 fc 04 64 dd 1f 92 89`

Syntactic decode only:
```asm
012B9FE0  ed                               in eax, dx
012B9FE1  49 72 65                         jb 0x12ba049
012B9FE4  8c cb                            mov ebx, cs
012B9FE6  bc 8c c9 7b f4                   mov esp, 0xf47bc98c
012B9FEB  26 c3                            ret 
012B9FED  c2 02 9c                         ret 0x9c02
012B9FF0  b3 71                            mov bl, 0x71
012B9FF2  18 fb                            sbb bl, bh
012B9FF4  f9                               stc 
012B9FF5  a9 b3 02 c5 9b                   test eax, 0x9bc502b3
012B9FFA  4a 7b d8                         jnp 0x12b9fd5
012B9FFD  e3 25                            jrcxz 0x12ba024
```

#### ID 1360149 at `0x20D05E0` (.text)

Bytes: `6f 91 81 fe ec 49 08 82 7e a6 db 51 da 51 42 ce c1 cd 09 62 86 dd c3 13 9a 15 53 54 8e b0 b4 7c 94 1a d2 b2 b7 83 29 11 63 82 04 83 5d 63 ac 70`

Syntactic decode only:
```asm
020D05E0  6f                               outsd dx, dword ptr [rsi]
020D05E1  91                               xchg ecx, eax
020D05E2  81 fe ec 49 08 82                cmp esi, 0x820849ec
020D05E8  7e a6                            jle 0x20d0590
020D05EA  db 51 da                         fist dword ptr [rcx - 0x26]
020D05ED  51                               push rcx
```

#### ID 827659 at `0x20A5B50` (.text)

Bytes: `95 f8 92 b3 fa 24 b2 c7 a3 35 c8 0c d3 cf 92 5f fd d0 b6 79 e3 3d 97 d9 f4 71 f9 28 52 b1 99 37 5b 64 52 62 36 16 72 e1 30 d6 e7 9e 89 c8 8b c6`

Syntactic decode only:
```asm
020A5B50  95                               xchg ebp, eax
020A5B51  f8                               clc 
020A5B52  92                               xchg edx, eax
020A5B53  b3 fa                            mov bl, 0xfa
020A5B55  24 b2                            and al, 0xb2
```

#### ID 457340 at `0x89DC0` (.text)

Bytes: `cb 0c 43 28 9d 6f 5d 61 bd 9d d7 a9 95 92 8d ce 19 99 4b cc c1 f1 a1 8f d3 29 b6 e7 61 e7 d5 5a 48 3c eb 80 6a f8 fa cf 10 04 76 1d 35 4c 0e 54`

Syntactic decode only:
```asm
00089DC0  cb                               retf 
00089DC1  0c 43                            or al, 0x43
00089DC3  28 9d 6f 5d 61 bd                sub byte ptr [rbp - 0x429ea291], bl
00089DC9  9d                               popfq 
00089DCA  d7                               xlatb 
00089DCB  a9 95 92 8d ce                   test eax, 0xce8d9295
00089DD0  19 99 4b cc c1 f1                sbb dword ptr [rcx - 0xe3e33b5], ebx
00089DD6  a1 8f d3 29 b6 e7 61 e7 d5       movabs eax, dword ptr [0xd5e761e7b629d38f]
00089DDF  5a                               pop rdx
00089DE0  48 3c eb                         cmp al, 0xeb
00089DE3  80 6a f8 fa                      sub byte ptr [rdx - 8], 0xfa
00089DE7  cf                               iretd 
```

#### ID 657534 at `0x28DA80` (.text)

Bytes: `b6 9d 53 db 96 af 51 8c d7 df db 1c 29 a7 87 fd 88 d3 79 35 de b0 62 c5 74 f9 e5 ab 6b 2e 5f 83 04 8d c0 8a 6b e2 10 b5 af ec 89 d6 f7 52 4f c6`

Syntactic decode only:
```asm
0028DA80  b6 9d                            mov dh, 0x9d
0028DA82  53                               push rbx
0028DA83  db 96 af 51 8c d7                fist dword ptr [rsi - 0x2873ae51]
```

#### ID 523613 at `0x14FAF0` (.text)

Bytes: `18 72 d2 f3 71 13 14 f7 ad 92 61 0b 2f 9b b8 b5 f5 4a 41 98 b4 fa 8d a3 e8 ee 5b 55 10 60 50 a4 c4 b6 13 49 be 86 6f 1f 68 30 56 08 1f 07 0c 88`

Syntactic decode only:
```asm
0014FAF0  18 72 d2                         sbb byte ptr [rdx - 0x2e], dh
0014FAF3  f3 71 13                         jno 0x14fb09
0014FAF6  14 f7                            adc al, 0xf7
0014FAF8  ad                               lodsd eax, dword ptr [rsi]
0014FAF9  92                               xchg edx, eax
```

#### ID 139753 at `0x5DD850` (.text)

Bytes: `0c ed 00 81 75 cb 76 8d 79 fc cb 77 8d a3 82 21 7e 45 c7 dd 25 bf 34 a9 d3 c3 fb 28 59 ba 55 af 9e 34 23 80 90 1b e5 ab 6e 52 82 64 0b d7 77 fd`

Syntactic decode only:
```asm
005DD850  0c ed                            or al, 0xed
005DD852  00 81 75 cb 76 8d                add byte ptr [rcx - 0x7289348b], al
005DD858  79 fc                            jns 0x5dd856
005DD85A  cb                               retf 
005DD85B  77 8d                            ja 0x5dd7ea
005DD85D  a3 82 21 7e 45 c7 dd 25 bf       movabs dword ptr [0xbf25ddc7457e2182], eax
005DD866  34 a9                            xor al, 0xa9
005DD868  d3 c3                            rol ebx, cl
005DD86A  fb                               sti 
005DD86B  28 59 ba                         sub byte ptr [rcx - 0x46], bl
005DD86E  55                               push rbp
005DD86F  af                               scasd eax, dword ptr [rdi]
```

#### ID 487988 at `0x2C4C1B8` (.rdata)

Bytes: `90 27 05 40 01 00 00 00 60 8e 04 40 01 00 00 00 e0 96 15 40 01 00 00 00 70 97 15 40 01 00 00 00 a0 6d 04 40 01 00 00 00 c0 6d 04 40 01 00 00 00`

Slot 0: bytes `90 27 05 40 01 00 00 00`; pointer VA `0x140052790`; candidate target RVA `0x52790` (.text).
```asm
00052790  cd 96                            int 0x96
00052792  68 3a d0 e9 0b                   push 0xbe9d03a
00052797  44 90                            nop 
00052799  88 d0                            mov al, dl
0005279B  9c                               pushfq 
0005279C  fa                               cli 
0005279D  38 7d e9                         cmp byte ptr [rbp - 0x17], bh
000527A0  9e                               sahf 
000527A1  40 7a f7                         jp 0x5279b
000527A4  1d a0 7d ec d1                   sbb eax, 0xd1ec7da0
000527A9  1c 0b                            sbb al, 0xb
000527AB  ca 74 1e                         retf 0x1e74
000527AE  21 d7                            and edi, edx
```

Slot 6: bytes `80 97 15 40 01 00 00 00`; pointer VA `0x140159780`; candidate target RVA `0x159780` (.text).
```asm
(no decode)
```

#### ID 1381610 at `0x2C788C8` (.rdata)

Bytes: `34 d7 31 40 01 00 00 00 60 8e 04 40 01 00 00 00 70 b6 15 40 01 00 00 00 90 b6 15 40 01 00 00 00 d0 ce 15 40 01 00 00 00 c0 6d 04 40 01 00 00 00`

Slot 0: bytes `34 d7 31 40 01 00 00 00`; pointer VA `0x14031D734`; candidate target RVA `0x31d734` (.text).
```asm
0031D734  20 f2                            and dl, dh
```

Slot 10: bytes `00 c8 31 40 01 00 00 00`; pointer VA `0x14031C800`; candidate target RVA `0x31c800` (.text).
```asm
(no decode)
```

#### ID 718716 at `0x2C78588` (.rdata)

Bytes: `90 d8 31 40 01 00 00 00 60 8e 04 40 01 00 00 00 40 21 15 40 01 00 00 00 30 22 15 40 01 00 00 00 a0 6d 04 40 01 00 00 00 c0 6d 04 40 01 00 00 00`

Slot 0: bytes `90 d8 31 40 01 00 00 00`; pointer VA `0x14031D890`; candidate target RVA `0x31d890` (.text).
```asm
0031D890  a7                               cmpsd dword ptr [rsi], dword ptr [rdi]
0031D891  2d 33 c6 1a de                   sub eax, 0xde1ac633
0031D896  ef                               out dx, eax
0031D897  6d                               insd dword ptr [rdi], dx
```

Slot 10: bytes `e0 b0 06 40 01 00 00 00`; pointer VA `0x14006B0E0`; candidate target RVA `0x6b0e0` (.text).
```asm
0006B0E0  12 c3                            adc al, bl
0006B0E2  5a                               pop rdx
0006B0E3  39 5a ab                         cmp dword ptr [rdx - 0x55], ebx
0006B0E6  8c af 80 a9 01 92                mov word ptr [rdi - 0x6dfe5680], gs
0006B0EC  a7                               cmpsd dword ptr [rsi], dword ptr [rdi]
0006B0ED  3b 77 6e                         cmp esi, dword ptr [rdi + 0x6e]
0006B0F0  0c 1a                            or al, 0x1a
0006B0F2  6d                               insd dword ptr [rdi], dx
0006B0F3  7b 28                            jnp 0x6b11d
0006B0F5  95                               xchg ebp, eax
0006B0F6  cc                               int3 
0006B0F7  4d 14 b6                         adc al, 0xb6
0006B0FA  e7 83                            out 0x83, eax
0006B0FC  f7 4b 5c 71 e8 f1 04             test dword ptr [rbx + 0x5c], 0x4f1e871
0006B103  22 31                            and dh, byte ptr [rcx]
0006B105  4f ae                            scasb al, byte ptr [rdi]
0006B107  88 72 65                         mov byte ptr [rdx + 0x65], dh
0006B10A  74 90                            je 0x6b09c
0006B10C  33 32                            xor esi, dword ptr [rdx]
0006B10E  0b 38                            or edi, dword ptr [rax]
```

#### ID 846981 at `0x2CB6258` (.rdata)

Bytes: `10 fb 56 40 01 00 00 00 50 37 57 40 01 00 00 00 70 37 57 40 01 00 00 00 c0 31 57 40 01 00 00 00 10 38 57 40 01 00 00 00 60 fe 56 40 01 00 00 00`

Slot 0: bytes `10 fb 56 40 01 00 00 00`; pointer VA `0x14056FB10`; candidate target RVA `0x56fb10` (.text).
```asm
0056FB10  b3 38                            mov bl, 0x38
```

Slot 7: bytes `c0 ff 56 40 01 00 00 00`; pointer VA `0x14056FFC0`; candidate target RVA `0x56ffc0` (.text).
```asm
0056FFC0  8a 71 c3                         mov dh, byte ptr [rcx - 0x3d]
0056FFC3  73 88                            jae 0x56ff4d
0056FFC5  40 69 66 2d cf 83 ae 56          imul esp, dword ptr [rsi + 0x2d], 0x56ae83cf
0056FFCD  39 11                            cmp dword ptr [rcx], edx
0056FFCF  e7 1d                            out 0x1d, eax
0056FFD1  a8 14                            test al, 0x14
```

#### ID 859152 at `0x2CB4498` (.rdata)

Bytes: `d0 34 56 40 01 00 00 00 70 38 56 40 01 00 00 00 f0 28 56 40 01 00 00 00 d0 20 56 40 01 00 00 00 10 38 56 40 01 00 00 00 90 29 56 40 01 00 00 00`

Slot 0: bytes `d0 34 56 40 01 00 00 00`; pointer VA `0x1405634D0`; candidate target RVA `0x5634d0` (.text).
```asm
005634D0  09 0a                            or dword ptr [rdx], ecx
005634D2  00 90 f4 11 3e 64                add byte ptr [rax + 0x643e11f4], dl
005634D8  ef                               out dx, eax
005634D9  92                               xchg edx, eax
```

Slot 5: bytes `90 29 56 40 01 00 00 00`; pointer VA `0x140562990`; candidate target RVA `0x562990` (.text).
```asm
00562990  b7 9e                            mov bh, 0x9e
00562992  e9 95 9c f4 dd                   jmp 0xffffffffde4ac62c
00562997  b6 5e                            mov dh, 0x5e
00562999  c9                               leave 
0056299A  e5 4b                            in eax, 0x4b
0056299C  5e                               pop rsi
```

#### Dialogue.cpp pattern

Pattern `48 83 C6 18 48 83 C7 18`; file-backed .text hit count **0**. Entire min(VirtualSize, RawSize) searched. Protected on-disk zero hits do not establish loaded-image absence.

### 1-11-191-0: `Fallout4.exe`

Size 55290280; SHA-256 `81694b37816c8045855905a52c5fb13583c5803121fabb5024760892014e9bc6`; ImageBase `0x140000000`.

| Section | RVA | Virtual size | File offset | Raw size | Flags | Entropy bits/byte |
|---|---:|---:|---:|---:|---:|---:|
| .text | 0x1000 | 0x242B1CC | 0x600 | 0x242B200 | 0x60000020 | 6.47457 |
| .interpr | 0x242D000 | 0xA6D0 | 0x242B800 | 0xA800 | 0x60000020 | 6.40587 |
| .rdata | 0x2438000 | 0xA94B42 | 0x2436000 | 0xA94C00 | 0x40000040 | 5.26377 |
| .data | 0x2ECD000 | 0xF92330 | 0x2ECAC00 | 0x20D200 | 0xC0000040 | 4.82170 |
| .pdata | 0x3E60000 | 0x27E3D0 | 0x30D7E00 | 0x27E400 | 0x40000040 | 7.04474 |
| _RDATA | 0x40DF000 | 0xE20 | 0x3356200 | 0x1000 | 0x40000040 | 6.06713 |
| .rsrc | 0x40E0000 | 0xA67A0 | 0x3357200 | 0xA6800 | 0x40000040 | 5.10135 |
| .reloc | 0x4187000 | 0x82158 | 0x33FDA00 | 0x82200 | 0x42000040 | 5.47144 |
| .bind | 0x420A000 | 0x38448 | 0x347FC00 | 0x38448 | 0x60000000 | 7.95855 |

#### ID 487988 at `0x24625E0` (.rdata)

Bytes: `b0 55 25 40 01 00 00 00 50 ee 24 40 01 00 00 00 d0 5a 31 40 01 00 00 00 80 5b 31 40 01 00 00 00 b0 da 24 40 01 00 00 00 d0 da 24 40 01 00 00 00`

Slot 0: bytes `b0 55 25 40 01 00 00 00`; pointer VA `0x1402555B0`; candidate target RVA `0x2555b0` (.text).
```asm
002555B0  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
002555B5  57                               push rdi
002555B6  48 83 ec 20                      sub rsp, 0x20
002555BA  48 8b f9                         mov rdi, rcx
002555BD  8b da                            mov ebx, edx
002555BF  48 83 c1 08                      add rcx, 8
002555C3  e8 18 73 42 01                   call 0x167c8e0
002555C8  48 8d 05 e9 cd 20 02             lea rax, [rip + 0x220cde9]
002555CF  48 89 07                         mov qword ptr [rdi], rax
002555D2  f6 c3 01                         test bl, 1
002555D5  74 0d                            je 0x2555e4
002555D7  ba 10 00 00 00                   mov edx, 0x10
002555DC  48 8b cf                         mov rcx, rdi
002555DF  e8 e4 15 06 02                   call 0x22b6bc8
002555E4  48 8b 5c 24 30                   mov rbx, qword ptr [rsp + 0x30]
002555E9  48 8b c7                         mov rax, rdi
002555EC  48 83 c4 20                      add rsp, 0x20
002555F0  5f                               pop rdi
002555F1  c3                               ret 
002555F2  cc                               int3 
```

Slot 6: bytes `90 5b 31 40 01 00 00 00`; pointer VA `0x140315B90`; candidate target RVA `0x315b90` (.text).
```asm
00315B90  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
00315B95  48 89 74 24 10                   mov qword ptr [rsp + 0x10], rsi
00315B9A  57                               push rdi
00315B9B  48 83 ec 30                      sub rsp, 0x30
00315B9F  44 8b 05 b2 6a b4 03             mov r8d, dword ptr [rip + 0x3b46ab2]
00315BA6  4c 8d 0d 83 db c7 02             lea r9, [rip + 0x2c7db83]
00315BAD  65 48 8b 04 25 58 00 00 00       mov rax, qword ptr gs:[0x58]
00315BB6  48 8b f1                         mov rsi, rcx
00315BB9  4c 8b d2                         mov r10, rdx
00315BBC  b9 c0 09 00 00                   mov ecx, 0x9c0
00315BC1  33 d2                            xor edx, edx
00315BC3  c7 44 24 20 00 00 00 00          mov dword ptr [rsp + 0x20], 0
00315BCB  4a 8b 3c c0                      mov rdi, qword ptr [rax + r8*8]
00315BCF  4c 8d 05 22 d9 c7 02             lea r8, [rip + 0x2c7d922]
00315BD6  48 03 f9                         add rdi, rcx
00315BD9  49 8b ca                         mov rcx, r10
00315BDC  8b 1f                            mov ebx, dword ptr [rdi]
00315BDE  c7 07 62 00 00 00                mov dword ptr [rdi], 0x62
00315BE4  e8 b5 6f fa 01                   call 0x22bcb9e
00315BE9  48 85 c0                         test rax, rax
```

#### ID 1381610 at `0x248D340` (.rdata)

Bytes: `38 1c 45 40 01 00 00 00 50 ee 24 40 01 00 00 00 10 6d 31 40 01 00 00 00 30 6d 31 40 01 00 00 00 50 85 31 40 01 00 00 00 d0 da 24 40 01 00 00 00`

Slot 0: bytes `38 1c 45 40 01 00 00 00`; pointer VA `0x140451C38`; candidate target RVA `0x451c38` (.text).
```asm
00451C38  48 83 e9 68                      sub rcx, 0x68
00451C3C  e9 0f 00 00 00                   jmp 0x451c50
00451C41  cc                               int3 
00451C42  cc                               int3 
00451C43  cc                               int3 
00451C44  cc                               int3 
00451C45  cc                               int3 
00451C46  cc                               int3 
00451C47  cc                               int3 
00451C48  cc                               int3 
00451C49  cc                               int3 
00451C4A  cc                               int3 
00451C4B  cc                               int3 
00451C4C  cc                               int3 
00451C4D  cc                               int3 
00451C4E  cc                               int3 
00451C4F  cc                               int3 
00451C50  40 57                            push rdi
00451C52  48 83 ec 30                      sub rsp, 0x30
00451C56  48 8d 05 a3 b3 03 02             lea rax, [rip + 0x203b3a3]
```

Slot 10: bytes `a0 11 45 40 01 00 00 00`; pointer VA `0x1404511A0`; candidate target RVA `0x4511a0` (.text).
```asm
004511A0  40 53                            push rbx
004511A2  48 83 ec 20                      sub rsp, 0x20
004511A6  48 8b d9                         mov rbx, rcx
004511A9  48 8d 4c 24 38                   lea rcx, [rsp + 0x38]
004511AE  e8 7d fe ef ff                   call 0x351030
004511B3  48 83 c3 98                      add rbx, -0x68
004511B7  48 8d 4c 24 30                   lea rcx, [rsp + 0x30]
004511BC  48 8b d3                         mov rdx, rbx
004511BF  45 33 c0                         xor r8d, r8d
004511C2  e8 19 7d 20 01                   call 0x1658ee0
004511C7  48 8b 15 2a 4e a8 02             mov rdx, qword ptr [rip + 0x2a84e2a]
004511CE  48 85 d2                         test rdx, rdx
004511D1  74 55                            je 0x451228
004511D3  8b 0d 03 4e a8 02                mov ecx, dword ptr [rip + 0x2a84e03]
004511D9  8b 44 24 30                      mov eax, dword ptr [rsp + 0x30]
004511DD  ff c9                            dec ecx
004511DF  48 23 c8                         and rcx, rax
004511E2  48 8d 04 49                      lea rax, [rcx + rcx*2]
004511E6  48 8d 14 c2                      lea rdx, [rdx + rax*8]
004511EA  48 85 d2                         test rdx, rdx
```

#### ID 718716 at `0x248D000` (.rdata)

Bytes: `50 1c 45 40 01 00 00 00 50 ee 24 40 01 00 00 00 c0 0b 31 40 01 00 00 00 b0 0c 31 40 01 00 00 00 b0 da 24 40 01 00 00 00 d0 da 24 40 01 00 00 00`

Slot 0: bytes `50 1c 45 40 01 00 00 00`; pointer VA `0x140451C50`; candidate target RVA `0x451c50` (.text).
```asm
00451C50  40 57                            push rdi
00451C52  48 83 ec 30                      sub rsp, 0x30
00451C56  48 8d 05 a3 b3 03 02             lea rax, [rip + 0x203b3a3]
00451C5D  48 89 6c 24 50                   mov qword ptr [rsp + 0x50], rbp
00451C62  48 89 01                         mov qword ptr [rcx], rax
00451C65  8b ea                            mov ebp, edx
00451C67  48 8d 05 d2 b6 03 02             lea rax, [rip + 0x203b6d2]
00451C6E  4c 89 74 24 20                   mov qword ptr [rsp + 0x20], r14
00451C73  48 89 41 68                      mov qword ptr [rcx + 0x68], rax
00451C77  48 8b f9                         mov rdi, rcx
00451C7A  48 8b d1                         mov rdx, rcx
00451C7D  45 33 c0                         xor r8d, r8d
00451C80  48 8d 4c 24 48                   lea rcx, [rsp + 0x48]
00451C85  e8 56 72 20 01                   call 0x1658ee0
00451C8A  4c 8b 05 67 43 a8 02             mov r8, qword ptr [rip + 0x2a84367]
00451C91  4d 85 c0                         test r8, r8
00451C94  0f 84 d5 00 00 00                je 0x451d6f
00451C9A  8b 0d 3c 43 a8 02                mov ecx, dword ptr [rip + 0x2a8433c]
00451CA0  8b 44 24 48                      mov eax, dword ptr [rsp + 0x48]
00451CA4  ff c9                            dec ecx
```

Slot 10: bytes `50 24 26 40 01 00 00 00`; pointer VA `0x140262450`; candidate target RVA `0x262450` (.text).
```asm
00262450  b0 01                            mov al, 1
00262452  c3                               ret 
00262453  cc                               int3 
00262454  cc                               int3 
00262455  cc                               int3 
00262456  cc                               int3 
00262457  cc                               int3 
00262458  cc                               int3 
00262459  cc                               int3 
0026245A  cc                               int3 
0026245B  cc                               int3 
0026245C  cc                               int3 
0026245D  cc                               int3 
0026245E  cc                               int3 
0026245F  cc                               int3 
00262460  48 83 ec 38                      sub rsp, 0x38
00262464  48 8b 01                         mov rax, qword ptr [rcx]
00262467  45 33 c9                         xor r9d, r9d
0026246A  c7 44 24 20 01 00 00 00          mov dword ptr [rsp + 0x20], 1
00262472  ff 90 00 02 00 00                call qword ptr [rax + 0x200]
```

#### ID 846981 at `0x24CA3D8` (.rdata)

Bytes: `40 31 62 40 01 00 00 00 f0 5f 62 40 01 00 00 00 10 60 62 40 01 00 00 00 d0 5b 62 40 01 00 00 00 40 60 62 40 01 00 00 00 f0 35 62 40 01 00 00 00`

Slot 0: bytes `40 31 62 40 01 00 00 00`; pointer VA `0x140623140`; candidate target RVA `0x623140` (.text).
```asm
00623140  4c 89 4c 24 20                   mov qword ptr [rsp + 0x20], r9
00623145  48 89 4c 24 08                   mov qword ptr [rsp + 8], rcx
0062314A  55                               push rbp
0062314B  41 54                            push r12
0062314D  41 56                            push r14
0062314F  48 83 ec 70                      sub rsp, 0x70
00623153  0f b6 41 12                      movzx eax, byte ptr [rcx + 0x12]
00623157  49 8b e9                         mov rbp, r9
0062315A  44 8b f2                         mov r14d, edx
0062315D  4d 8b e0                         mov r12, r8
00623160  3b d0                            cmp edx, eax
00623162  74 0c                            je 0x623170
00623164  32 c0                            xor al, al
00623166  48 83 c4 70                      add rsp, 0x70
0062316A  41 5e                            pop r14
0062316C  41 5c                            pop r12
0062316E  5d                               pop rbp
0062316F  c3                               ret 
00623170  48 89 9c 24 98 00 00 00          mov qword ptr [rsp + 0x98], rbx
00623178  48 8d 4c 24 48                   lea rcx, [rsp + 0x48]
```

Slot 7: bytes `30 37 62 40 01 00 00 00`; pointer VA `0x140623730`; candidate target RVA `0x623730` (.text).
```asm
00623730  48 89 5c 24 10                   mov qword ptr [rsp + 0x10], rbx
00623735  55                               push rbp
00623736  57                               push rdi
00623737  41 54                            push r12
00623739  41 56                            push r14
0062373B  41 57                            push r15
0062373D  48 83 ec 20                      sub rsp, 0x20
00623741  4c 8b f2                         mov r14, rdx
00623744  c6 44 24 60 00                   mov byte ptr [rsp + 0x60], 0
00623749  4c 8b f9                         mov r15, rcx
0062374C  48 89 74 24 50                   mov qword ptr [rsp + 0x50], rsi
00623751  40 b5 01                         mov bpl, 1
00623754  49 c7 c4 ff ff ff ff             mov r12, 0xffffffffffffffff
0062375B  0f 1f 44 00 00                   nop dword ptr [rax + rax]
00623760  40 84 ed                         test bpl, bpl
00623763  0f 84 11 02 00 00                je 0x62397a
00623769  49 8b ce                         mov rcx, r14
0062376C  e8 3f 4e cd ff                   call 0x2f85b0
00623771  85 c0                            test eax, eax
00623773  0f 84 01 02 00 00                je 0x62397a
```

#### ID 859152 at `0x24C8688` (.rdata)

Bytes: `e0 a1 61 40 01 00 00 00 30 a4 61 40 01 00 00 00 60 9f 61 40 01 00 00 00 80 95 61 40 01 00 00 00 10 a4 61 40 01 00 00 00 00 a0 61 40 01 00 00 00`

Slot 0: bytes `e0 a1 61 40 01 00 00 00`; pointer VA `0x14061A1E0`; candidate target RVA `0x61a1e0` (.text).
```asm
0061A1E0  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
0061A1E5  57                               push rdi
0061A1E6  48 83 ec 20                      sub rsp, 0x20
0061A1EA  48 8d 05 97 e4 ea 01             lea rax, [rip + 0x1eae497]
0061A1F1  48 8b f9                         mov rdi, rcx
0061A1F4  48 89 01                         mov qword ptr [rcx], rax
0061A1F7  8b da                            mov ebx, edx
0061A1F9  48 83 c1 18                      add rcx, 0x18
0061A1FD  e8 de 26 06 01                   call 0x167c8e0
0061A202  48 8d 05 bf e2 ea 01             lea rax, [rip + 0x1eae2bf]
0061A209  48 89 07                         mov qword ptr [rdi], rax
0061A20C  f6 c3 01                         test bl, 1
0061A20F  74 0d                            je 0x61a21e
0061A211  ba 30 00 00 00                   mov edx, 0x30
0061A216  48 8b cf                         mov rcx, rdi
0061A219  e8 aa c9 c9 01                   call 0x22b6bc8
0061A21E  48 8b 5c 24 30                   mov rbx, qword ptr [rsp + 0x30]
0061A223  48 8b c7                         mov rax, rdi
0061A226  48 83 c4 20                      add rsp, 0x20
0061A22A  5f                               pop rdi
```

Slot 5: bytes `00 a0 61 40 01 00 00 00`; pointer VA `0x14061A000`; candidate target RVA `0x61a000` (.text).
```asm
0061A000  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
0061A005  48 89 74 24 10                   mov qword ptr [rsp + 0x10], rsi
0061A00A  57                               push rdi
0061A00B  48 83 ec 20                      sub rsp, 0x20
0061A00F  48 8b f9                         mov rdi, rcx
0061A012  48 8b da                         mov rbx, rdx
0061A015  48 8b ca                         mov rcx, rdx
0061A018  40 b6 01                         mov sil, 1
0061A01B  e8 90 e5 cd ff                   call 0x2f85b0
0061A020  3d 45 50 46 32                   cmp eax, 0x32465045
0061A025  0f 84 10 01 00 00                je 0x61a13b
0061A02B  3d 45 50 46 33                   cmp eax, 0x33465045
0061A030  74 53                            je 0x61a085
0061A032  3d 45 50 46 44                   cmp eax, 0x44465045
0061A037  0f 85 0a 01 00 00                jne 0x61a147
0061A03D  83 bb b8 02 00 00 04             cmp dword ptr [rbx + 0x2b8], 4
0061A044  40 0f 94 c6                      sete sil
0061A048  0f 85 f9 00 00 00                jne 0x61a147
0061A04E  41 b8 04 00 00 00                mov r8d, 4
0061A054  c7 44 24 40 00 00 00 00          mov dword ptr [rsp + 0x40], 0
```

#### Dialogue.cpp pattern

Pattern `48 83 C6 18 48 83 C7 18`; file-backed .text hit count **1**. Entire min(VirtualSize, RawSize) searched. Protected on-disk zero hits do not establish loaded-image absence.

Hit `0x215041F`; +0xC instruction `0x215042B` bytes `49 8b b6 88 00 00 00`; signed disp32 136 (`0x00000088`); operand RVA `0x21504BA`; operand - 0x28 = map candidate RVA `0x2150492` (.text). Container ABI UNVERIFIED.
```asm
0215040F  00 00                            add byte ptr [rax], al
02150411  00 e8                            add al, ch
02150413  d9 ba 52 ff 48 8b                fnstcw word ptr [rdx - 0x74b700ae]
```

### 1-11-221-0: `Fallout4.exe`

Size 55293864; SHA-256 `428f9996cc4248e26c0f62f9fdd3eaf0e5eb305834b67ee5996538e593218b61`; ImageBase `0x140000000`.

| Section | RVA | Virtual size | File offset | Raw size | Flags | Entropy bits/byte |
|---|---:|---:|---:|---:|---:|---:|
| .text | 0x1000 | 0x242B2BC | 0x600 | 0x242B400 | 0x60000020 | 6.47458 |
| .interpr | 0x242D000 | 0xA6D0 | 0x242BA00 | 0xA800 | 0x60000020 | 6.40492 |
| .rdata | 0x2438000 | 0xA94BE2 | 0x2436200 | 0xA94C00 | 0x40000040 | 5.26374 |
| .data | 0x2ECD000 | 0xF923B0 | 0x2ECAE00 | 0x20D200 | 0xC0000040 | 4.82870 |
| .pdata | 0x3E60000 | 0x27E400 | 0x30D8000 | 0x27E400 | 0x40000040 | 7.04443 |
| _RDATA | 0x40DF000 | 0xE20 | 0x3356400 | 0x1000 | 0x40000040 | 6.06713 |
| .rsrc | 0x40E0000 | 0xA67A0 | 0x3357400 | 0xA6800 | 0x40000040 | 5.10135 |
| .reloc | 0x4187000 | 0x8219C | 0x33FDC00 | 0x82200 | 0x42000040 | 5.47136 |
| .bind | 0x420A000 | 0x39048 | 0x347FE00 | 0x39048 | 0x60000000 | 7.95921 |

#### ID 487988 at `0x24625E0` (.rdata)

Bytes: `b0 55 25 40 01 00 00 00 50 ee 24 40 01 00 00 00 e0 5a 31 40 01 00 00 00 90 5b 31 40 01 00 00 00 b0 da 24 40 01 00 00 00 d0 da 24 40 01 00 00 00`

Slot 0: bytes `b0 55 25 40 01 00 00 00`; pointer VA `0x1402555B0`; candidate target RVA `0x2555b0` (.text).
```asm
002555B0  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
002555B5  57                               push rdi
002555B6  48 83 ec 20                      sub rsp, 0x20
002555BA  48 8b f9                         mov rdi, rcx
002555BD  8b da                            mov ebx, edx
002555BF  48 83 c1 08                      add rcx, 8
002555C3  e8 18 74 42 01                   call 0x167c9e0
002555C8  48 8d 05 e9 cd 20 02             lea rax, [rip + 0x220cde9]
002555CF  48 89 07                         mov qword ptr [rdi], rax
002555D2  f6 c3 01                         test bl, 1
002555D5  74 0d                            je 0x2555e4
002555D7  ba 10 00 00 00                   mov edx, 0x10
002555DC  48 8b cf                         mov rcx, rdi
002555DF  e8 c4 16 06 02                   call 0x22b6ca8
002555E4  48 8b 5c 24 30                   mov rbx, qword ptr [rsp + 0x30]
002555E9  48 8b c7                         mov rax, rdi
002555EC  48 83 c4 20                      add rsp, 0x20
002555F0  5f                               pop rdi
002555F1  c3                               ret 
002555F2  cc                               int3 
```

Slot 6: bytes `a0 5b 31 40 01 00 00 00`; pointer VA `0x140315BA0`; candidate target RVA `0x315ba0` (.text).
```asm
00315BA0  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
00315BA5  48 89 74 24 10                   mov qword ptr [rsp + 0x10], rsi
00315BAA  57                               push rdi
00315BAB  48 83 ec 30                      sub rsp, 0x30
00315BAF  44 8b 05 22 6b b4 03             mov r8d, dword ptr [rip + 0x3b46b22]
00315BB6  4c 8d 0d b3 db c7 02             lea r9, [rip + 0x2c7dbb3]
00315BBD  65 48 8b 04 25 58 00 00 00       mov rax, qword ptr gs:[0x58]
00315BC6  48 8b f1                         mov rsi, rcx
00315BC9  4c 8b d2                         mov r10, rdx
00315BCC  b9 c0 09 00 00                   mov ecx, 0x9c0
00315BD1  33 d2                            xor edx, edx
00315BD3  c7 44 24 20 00 00 00 00          mov dword ptr [rsp + 0x20], 0
00315BDB  4a 8b 3c c0                      mov rdi, qword ptr [rax + r8*8]
00315BDF  4c 8d 05 52 d9 c7 02             lea r8, [rip + 0x2c7d952]
00315BE6  48 03 f9                         add rdi, rcx
00315BE9  49 8b ca                         mov rcx, r10
00315BEC  8b 1f                            mov ebx, dword ptr [rdi]
00315BEE  c7 07 62 00 00 00                mov dword ptr [rdi], 0x62
00315BF4  e8 85 70 fa 01                   call 0x22bcc7e
00315BF9  48 85 c0                         test rax, rax
```

#### ID 1381610 at `0x248D340` (.rdata)

Bytes: `48 1c 45 40 01 00 00 00 50 ee 24 40 01 00 00 00 20 6d 31 40 01 00 00 00 40 6d 31 40 01 00 00 00 60 85 31 40 01 00 00 00 d0 da 24 40 01 00 00 00`

Slot 0: bytes `48 1c 45 40 01 00 00 00`; pointer VA `0x140451C48`; candidate target RVA `0x451c48` (.text).
```asm
00451C48  48 83 e9 68                      sub rcx, 0x68
00451C4C  e9 0f 00 00 00                   jmp 0x451c60
00451C51  cc                               int3 
00451C52  cc                               int3 
00451C53  cc                               int3 
00451C54  cc                               int3 
00451C55  cc                               int3 
00451C56  cc                               int3 
00451C57  cc                               int3 
00451C58  cc                               int3 
00451C59  cc                               int3 
00451C5A  cc                               int3 
00451C5B  cc                               int3 
00451C5C  cc                               int3 
00451C5D  cc                               int3 
00451C5E  cc                               int3 
00451C5F  cc                               int3 
00451C60  40 57                            push rdi
00451C62  48 83 ec 30                      sub rsp, 0x30
00451C66  48 8d 05 93 b3 03 02             lea rax, [rip + 0x203b393]
```

Slot 10: bytes `b0 11 45 40 01 00 00 00`; pointer VA `0x1404511B0`; candidate target RVA `0x4511b0` (.text).
```asm
004511B0  40 53                            push rbx
004511B2  48 83 ec 20                      sub rsp, 0x20
004511B6  48 8b d9                         mov rbx, rcx
004511B9  48 8d 4c 24 38                   lea rcx, [rsp + 0x38]
004511BE  e8 7d fe ef ff                   call 0x351040
004511C3  48 83 c3 98                      add rbx, -0x68
004511C7  48 8d 4c 24 30                   lea rcx, [rsp + 0x30]
004511CC  48 8b d3                         mov rdx, rbx
004511CF  45 33 c0                         xor r8d, r8d
004511D2  e8 09 7e 20 01                   call 0x1658fe0
004511D7  48 8b 15 1a 4e a8 02             mov rdx, qword ptr [rip + 0x2a84e1a]
004511DE  48 85 d2                         test rdx, rdx
004511E1  74 55                            je 0x451238
004511E3  8b 0d f3 4d a8 02                mov ecx, dword ptr [rip + 0x2a84df3]
004511E9  8b 44 24 30                      mov eax, dword ptr [rsp + 0x30]
004511ED  ff c9                            dec ecx
004511EF  48 23 c8                         and rcx, rax
004511F2  48 8d 04 49                      lea rax, [rcx + rcx*2]
004511F6  48 8d 14 c2                      lea rdx, [rdx + rax*8]
004511FA  48 85 d2                         test rdx, rdx
```

#### ID 718716 at `0x248D000` (.rdata)

Bytes: `60 1c 45 40 01 00 00 00 50 ee 24 40 01 00 00 00 d0 0b 31 40 01 00 00 00 c0 0c 31 40 01 00 00 00 b0 da 24 40 01 00 00 00 d0 da 24 40 01 00 00 00`

Slot 0: bytes `60 1c 45 40 01 00 00 00`; pointer VA `0x140451C60`; candidate target RVA `0x451c60` (.text).
```asm
00451C60  40 57                            push rdi
00451C62  48 83 ec 30                      sub rsp, 0x30
00451C66  48 8d 05 93 b3 03 02             lea rax, [rip + 0x203b393]
00451C6D  48 89 6c 24 50                   mov qword ptr [rsp + 0x50], rbp
00451C72  48 89 01                         mov qword ptr [rcx], rax
00451C75  8b ea                            mov ebp, edx
00451C77  48 8d 05 c2 b6 03 02             lea rax, [rip + 0x203b6c2]
00451C7E  4c 89 74 24 20                   mov qword ptr [rsp + 0x20], r14
00451C83  48 89 41 68                      mov qword ptr [rcx + 0x68], rax
00451C87  48 8b f9                         mov rdi, rcx
00451C8A  48 8b d1                         mov rdx, rcx
00451C8D  45 33 c0                         xor r8d, r8d
00451C90  48 8d 4c 24 48                   lea rcx, [rsp + 0x48]
00451C95  e8 46 73 20 01                   call 0x1658fe0
00451C9A  4c 8b 05 57 43 a8 02             mov r8, qword ptr [rip + 0x2a84357]
00451CA1  4d 85 c0                         test r8, r8
00451CA4  0f 84 d5 00 00 00                je 0x451d7f
00451CAA  8b 0d 2c 43 a8 02                mov ecx, dword ptr [rip + 0x2a8432c]
00451CB0  8b 44 24 48                      mov eax, dword ptr [rsp + 0x48]
00451CB4  ff c9                            dec ecx
```

Slot 10: bytes `50 24 26 40 01 00 00 00`; pointer VA `0x140262450`; candidate target RVA `0x262450` (.text).
```asm
00262450  b0 01                            mov al, 1
00262452  c3                               ret 
00262453  cc                               int3 
00262454  cc                               int3 
00262455  cc                               int3 
00262456  cc                               int3 
00262457  cc                               int3 
00262458  cc                               int3 
00262459  cc                               int3 
0026245A  cc                               int3 
0026245B  cc                               int3 
0026245C  cc                               int3 
0026245D  cc                               int3 
0026245E  cc                               int3 
0026245F  cc                               int3 
00262460  48 83 ec 38                      sub rsp, 0x38
00262464  48 8b 01                         mov rax, qword ptr [rcx]
00262467  45 33 c9                         xor r9d, r9d
0026246A  c7 44 24 20 01 00 00 00          mov dword ptr [rsp + 0x20], 1
00262472  ff 90 00 02 00 00                call qword ptr [rax + 0x200]
```

#### ID 846981 at `0x24CA3E8` (.rdata)

Bytes: `60 31 62 40 01 00 00 00 10 60 62 40 01 00 00 00 30 60 62 40 01 00 00 00 f0 5b 62 40 01 00 00 00 60 60 62 40 01 00 00 00 10 36 62 40 01 00 00 00`

Slot 0: bytes `60 31 62 40 01 00 00 00`; pointer VA `0x140623160`; candidate target RVA `0x623160` (.text).
```asm
00623160  4c 89 4c 24 20                   mov qword ptr [rsp + 0x20], r9
00623165  48 89 4c 24 08                   mov qword ptr [rsp + 8], rcx
0062316A  55                               push rbp
0062316B  41 54                            push r12
0062316D  41 56                            push r14
0062316F  48 83 ec 70                      sub rsp, 0x70
00623173  0f b6 41 12                      movzx eax, byte ptr [rcx + 0x12]
00623177  49 8b e9                         mov rbp, r9
0062317A  44 8b f2                         mov r14d, edx
0062317D  4d 8b e0                         mov r12, r8
00623180  3b d0                            cmp edx, eax
00623182  74 0c                            je 0x623190
00623184  32 c0                            xor al, al
00623186  48 83 c4 70                      add rsp, 0x70
0062318A  41 5e                            pop r14
0062318C  41 5c                            pop r12
0062318E  5d                               pop rbp
0062318F  c3                               ret 
00623190  48 89 9c 24 98 00 00 00          mov qword ptr [rsp + 0x98], rbx
00623198  48 8d 4c 24 48                   lea rcx, [rsp + 0x48]
```

Slot 7: bytes `50 37 62 40 01 00 00 00`; pointer VA `0x140623750`; candidate target RVA `0x623750` (.text).
```asm
00623750  48 89 5c 24 10                   mov qword ptr [rsp + 0x10], rbx
00623755  55                               push rbp
00623756  57                               push rdi
00623757  41 54                            push r12
00623759  41 56                            push r14
0062375B  41 57                            push r15
0062375D  48 83 ec 20                      sub rsp, 0x20
00623761  4c 8b f2                         mov r14, rdx
00623764  c6 44 24 60 00                   mov byte ptr [rsp + 0x60], 0
00623769  4c 8b f9                         mov r15, rcx
0062376C  48 89 74 24 50                   mov qword ptr [rsp + 0x50], rsi
00623771  40 b5 01                         mov bpl, 1
00623774  49 c7 c4 ff ff ff ff             mov r12, 0xffffffffffffffff
0062377B  0f 1f 44 00 00                   nop dword ptr [rax + rax]
00623780  40 84 ed                         test bpl, bpl
00623783  0f 84 11 02 00 00                je 0x62399a
00623789  49 8b ce                         mov rcx, r14
0062378C  e8 2f 4e cd ff                   call 0x2f85c0
00623791  85 c0                            test eax, eax
00623793  0f 84 01 02 00 00                je 0x62399a
```

#### ID 859152 at `0x24C8698` (.rdata)

Bytes: `00 a2 61 40 01 00 00 00 50 a4 61 40 01 00 00 00 80 9f 61 40 01 00 00 00 a0 95 61 40 01 00 00 00 30 a4 61 40 01 00 00 00 20 a0 61 40 01 00 00 00`

Slot 0: bytes `00 a2 61 40 01 00 00 00`; pointer VA `0x14061A200`; candidate target RVA `0x61a200` (.text).
```asm
0061A200  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
0061A205  57                               push rdi
0061A206  48 83 ec 20                      sub rsp, 0x20
0061A20A  48 8d 05 87 e4 ea 01             lea rax, [rip + 0x1eae487]
0061A211  48 8b f9                         mov rdi, rcx
0061A214  48 89 01                         mov qword ptr [rcx], rax
0061A217  8b da                            mov ebx, edx
0061A219  48 83 c1 18                      add rcx, 0x18
0061A21D  e8 be 27 06 01                   call 0x167c9e0
0061A222  48 8d 05 af e2 ea 01             lea rax, [rip + 0x1eae2af]
0061A229  48 89 07                         mov qword ptr [rdi], rax
0061A22C  f6 c3 01                         test bl, 1
0061A22F  74 0d                            je 0x61a23e
0061A231  ba 30 00 00 00                   mov edx, 0x30
0061A236  48 8b cf                         mov rcx, rdi
0061A239  e8 6a ca c9 01                   call 0x22b6ca8
0061A23E  48 8b 5c 24 30                   mov rbx, qword ptr [rsp + 0x30]
0061A243  48 8b c7                         mov rax, rdi
0061A246  48 83 c4 20                      add rsp, 0x20
0061A24A  5f                               pop rdi
```

Slot 5: bytes `20 a0 61 40 01 00 00 00`; pointer VA `0x14061A020`; candidate target RVA `0x61a020` (.text).
```asm
0061A020  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
0061A025  48 89 74 24 10                   mov qword ptr [rsp + 0x10], rsi
0061A02A  57                               push rdi
0061A02B  48 83 ec 20                      sub rsp, 0x20
0061A02F  48 8b f9                         mov rdi, rcx
0061A032  48 8b da                         mov rbx, rdx
0061A035  48 8b ca                         mov rcx, rdx
0061A038  40 b6 01                         mov sil, 1
0061A03B  e8 80 e5 cd ff                   call 0x2f85c0
0061A040  3d 45 50 46 32                   cmp eax, 0x32465045
0061A045  0f 84 10 01 00 00                je 0x61a15b
0061A04B  3d 45 50 46 33                   cmp eax, 0x33465045
0061A050  74 53                            je 0x61a0a5
0061A052  3d 45 50 46 44                   cmp eax, 0x44465045
0061A057  0f 85 0a 01 00 00                jne 0x61a167
0061A05D  83 bb b8 02 00 00 04             cmp dword ptr [rbx + 0x2b8], 4
0061A064  40 0f 94 c6                      sete sil
0061A068  0f 85 f9 00 00 00                jne 0x61a167
0061A06E  41 b8 04 00 00 00                mov r8d, 4
0061A074  c7 44 24 40 00 00 00 00          mov dword ptr [rsp + 0x40], 0
```

#### Dialogue.cpp pattern

Pattern `48 83 C6 18 48 83 C7 18`; file-backed .text hit count **1**. Entire min(VirtualSize, RawSize) searched. Protected on-disk zero hits do not establish loaded-image absence.

Hit `0x21504FF`; +0xC instruction `0x215050B` bytes `49 8b b6 88 00 00 00`; signed disp32 136 (`0x00000088`); operand RVA `0x215059A`; operand - 0x28 = map candidate RVA `0x2150572` (.text). Container ABI UNVERIFIED.
```asm
021504EF  00 00                            add byte ptr [rax], al
021504F1  00 e8                            add al, ch
021504F3  f9                               stc 
021504F4  ba 52 ff 48 8b                   mov edx, 0x8b48ff52
```

### 1-11-240-0: `Fallout4.exe`

Size 55343016; SHA-256 `fdcef37ac1230af6d0b0050eb2142b139ef3a867b37b9211fb6edfcc646072f8`; ImageBase `0x140000000`.

| Section | RVA | Virtual size | File offset | Raw size | Flags | Entropy bits/byte |
|---|---:|---:|---:|---:|---:|---:|
| .text | 0x1000 | 0x2433AEC | 0x400 | 0x2433C00 | 0x60000020 | 6.47485 |
| .interpr | 0x2435000 | 0xA6D0 | 0x2434000 | 0xA800 | 0x60000020 | 6.40933 |
| .rdata | 0x2440000 | 0xA97362 | 0x243E800 | 0xA97400 | 0x40000040 | 5.26175 |
| .data | 0x2ED8000 | 0xF9DCF0 | 0x2ED5C00 | 0x20D400 | 0xC0000040 | 4.83328 |
| .pdata | 0x3E76000 | 0x27EF88 | 0x30E3000 | 0x27F000 | 0x40000040 | 7.04556 |
| _RDATA | 0x40F5000 | 0xE20 | 0x3362000 | 0x1000 | 0x40000040 | 6.06713 |
| .rsrc | 0x40F6000 | 0xA67A0 | 0x3363000 | 0xA6800 | 0x40000040 | 5.10130 |
| .reloc | 0x419D000 | 0x82294 | 0x3409800 | 0x82400 | 0x42000040 | 5.47149 |
| .bind | 0x4220000 | 0x39248 | 0x348BC00 | 0x39248 | 0x60000000 | 7.95921 |

#### ID 487988 at `0x246A640` (.rdata)

Bytes: `d0 58 25 40 01 00 00 00 70 f1 24 40 01 00 00 00 00 5e 31 40 01 00 00 00 b0 5e 31 40 01 00 00 00 d0 dd 24 40 01 00 00 00 f0 dd 24 40 01 00 00 00`

Slot 0: bytes `d0 58 25 40 01 00 00 00`; pointer VA `0x1402558D0`; candidate target RVA `0x2558d0` (.text).
```asm
002558D0  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
002558D5  57                               push rdi
002558D6  48 83 ec 20                      sub rsp, 0x20
002558DA  48 8b f9                         mov rdi, rcx
002558DD  8b da                            mov ebx, edx
002558DF  48 83 c1 08                      add rcx, 8
002558E3  e8 18 74 42 01                   call 0x167cd00
002558E8  48 8d 05 29 4b 21 02             lea rax, [rip + 0x2214b29]
002558EF  48 89 07                         mov qword ptr [rdi], rax
002558F2  f6 c3 01                         test bl, 1
002558F5  74 0d                            je 0x255904
002558F7  ba 10 00 00 00                   mov edx, 0x10
002558FC  48 8b cf                         mov rcx, rdi
002558FF  e8 d4 8d 06 02                   call 0x22be6d8
00255904  48 8b 5c 24 30                   mov rbx, qword ptr [rsp + 0x30]
00255909  48 8b c7                         mov rax, rdi
0025590C  48 83 c4 20                      add rsp, 0x20
00255910  5f                               pop rdi
00255911  c3                               ret 
00255912  cc                               int3 
```

Slot 6: bytes `c0 5e 31 40 01 00 00 00`; pointer VA `0x140315EC0`; candidate target RVA `0x315ec0` (.text).
```asm
00315EC0  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
00315EC5  48 89 74 24 10                   mov qword ptr [rsp + 0x10], rsi
00315ECA  57                               push rdi
00315ECB  48 83 ec 30                      sub rsp, 0x30
00315ECF  44 8b 05 42 d1 b5 03             mov r8d, dword ptr [rip + 0x3b5d142]
00315ED6  4c 8d 0d e3 88 c8 02             lea r9, [rip + 0x2c888e3]
00315EDD  65 48 8b 04 25 58 00 00 00       mov rax, qword ptr gs:[0x58]
00315EE6  48 8b f1                         mov rsi, rcx
00315EE9  4c 8b d2                         mov r10, rdx
00315EEC  b9 c0 09 00 00                   mov ecx, 0x9c0
00315EF1  33 d2                            xor edx, edx
00315EF3  c7 44 24 20 00 00 00 00          mov dword ptr [rsp + 0x20], 0
00315EFB  4a 8b 3c c0                      mov rdi, qword ptr [rax + r8*8]
00315EFF  4c 8d 05 82 86 c8 02             lea r8, [rip + 0x2c88682]
00315F06  48 03 f9                         add rdi, rcx
00315F09  49 8b ca                         mov rcx, r10
00315F0C  8b 1f                            mov ebx, dword ptr [rdi]
00315F0E  c7 07 62 00 00 00                mov dword ptr [rdi], 0x62
00315F14  e8 95 e7 fa 01                   call 0x22c46ae
00315F19  48 85 c0                         test rax, rax
```

#### ID 1381610 at `0x24953A0` (.rdata)

Bytes: `68 1f 45 40 01 00 00 00 70 f1 24 40 01 00 00 00 40 70 31 40 01 00 00 00 60 70 31 40 01 00 00 00 80 88 31 40 01 00 00 00 f0 dd 24 40 01 00 00 00`

Slot 0: bytes `68 1f 45 40 01 00 00 00`; pointer VA `0x140451F68`; candidate target RVA `0x451f68` (.text).
```asm
00451F68  48 83 e9 68                      sub rcx, 0x68
00451F6C  e9 0f 00 00 00                   jmp 0x451f80
00451F71  cc                               int3 
00451F72  cc                               int3 
00451F73  cc                               int3 
00451F74  cc                               int3 
00451F75  cc                               int3 
00451F76  cc                               int3 
00451F77  cc                               int3 
00451F78  cc                               int3 
00451F79  cc                               int3 
00451F7A  cc                               int3 
00451F7B  cc                               int3 
00451F7C  cc                               int3 
00451F7D  cc                               int3 
00451F7E  cc                               int3 
00451F7F  cc                               int3 
00451F80  40 57                            push rdi
00451F82  48 83 ec 30                      sub rsp, 0x30
00451F86  48 8d 05 d3 30 04 02             lea rax, [rip + 0x20430d3]
```

Slot 10: bytes `d0 14 45 40 01 00 00 00`; pointer VA `0x1404514D0`; candidate target RVA `0x4514d0` (.text).
```asm
004514D0  40 53                            push rbx
004514D2  48 83 ec 20                      sub rsp, 0x20
004514D6  48 8b d9                         mov rbx, rcx
004514D9  48 8d 4c 24 38                   lea rcx, [rsp + 0x38]
004514DE  e8 7d fe ef ff                   call 0x351360
004514E3  48 83 c3 98                      add rbx, -0x68
004514E7  48 8d 4c 24 30                   lea rcx, [rsp + 0x30]
004514EC  48 8b d3                         mov rdx, rbx
004514EF  45 33 c0                         xor r8d, r8d
004514F2  e8 09 7e 20 01                   call 0x1659300
004514F7  48 8b 15 fa fa a8 02             mov rdx, qword ptr [rip + 0x2a8fafa]
004514FE  48 85 d2                         test rdx, rdx
00451501  74 55                            je 0x451558
00451503  8b 0d d3 fa a8 02                mov ecx, dword ptr [rip + 0x2a8fad3]
00451509  8b 44 24 30                      mov eax, dword ptr [rsp + 0x30]
0045150D  ff c9                            dec ecx
0045150F  48 23 c8                         and rcx, rax
00451512  48 8d 04 49                      lea rax, [rcx + rcx*2]
00451516  48 8d 14 c2                      lea rdx, [rdx + rax*8]
0045151A  48 85 d2                         test rdx, rdx
```

#### ID 718716 at `0x2495060` (.rdata)

Bytes: `80 1f 45 40 01 00 00 00 70 f1 24 40 01 00 00 00 f0 0e 31 40 01 00 00 00 e0 0f 31 40 01 00 00 00 d0 dd 24 40 01 00 00 00 f0 dd 24 40 01 00 00 00`

Slot 0: bytes `80 1f 45 40 01 00 00 00`; pointer VA `0x140451F80`; candidate target RVA `0x451f80` (.text).
```asm
00451F80  40 57                            push rdi
00451F82  48 83 ec 30                      sub rsp, 0x30
00451F86  48 8d 05 d3 30 04 02             lea rax, [rip + 0x20430d3]
00451F8D  48 89 6c 24 50                   mov qword ptr [rsp + 0x50], rbp
00451F92  48 89 01                         mov qword ptr [rcx], rax
00451F95  8b ea                            mov ebp, edx
00451F97  48 8d 05 02 34 04 02             lea rax, [rip + 0x2043402]
00451F9E  4c 89 74 24 20                   mov qword ptr [rsp + 0x20], r14
00451FA3  48 89 41 68                      mov qword ptr [rcx + 0x68], rax
00451FA7  48 8b f9                         mov rdi, rcx
00451FAA  48 8b d1                         mov rdx, rcx
00451FAD  45 33 c0                         xor r8d, r8d
00451FB0  48 8d 4c 24 48                   lea rcx, [rsp + 0x48]
00451FB5  e8 46 73 20 01                   call 0x1659300
00451FBA  4c 8b 05 37 f0 a8 02             mov r8, qword ptr [rip + 0x2a8f037]
00451FC1  4d 85 c0                         test r8, r8
00451FC4  0f 84 d5 00 00 00                je 0x45209f
00451FCA  8b 0d 0c f0 a8 02                mov ecx, dword ptr [rip + 0x2a8f00c]
00451FD0  8b 44 24 48                      mov eax, dword ptr [rsp + 0x48]
00451FD4  ff c9                            dec ecx
```

Slot 10: bytes `70 27 26 40 01 00 00 00`; pointer VA `0x140262770`; candidate target RVA `0x262770` (.text).
```asm
00262770  b0 01                            mov al, 1
00262772  c3                               ret 
00262773  cc                               int3 
00262774  cc                               int3 
00262775  cc                               int3 
00262776  cc                               int3 
00262777  cc                               int3 
00262778  cc                               int3 
00262779  cc                               int3 
0026277A  cc                               int3 
0026277B  cc                               int3 
0026277C  cc                               int3 
0026277D  cc                               int3 
0026277E  cc                               int3 
0026277F  cc                               int3 
00262780  48 83 ec 38                      sub rsp, 0x38
00262784  48 8b 01                         mov rax, qword ptr [rcx]
00262787  45 33 c9                         xor r9d, r9d
0026278A  c7 44 24 20 01 00 00 00          mov dword ptr [rsp + 0x20], 1
00262792  ff 90 00 02 00 00                call qword ptr [rax + 0x200]
```

#### ID 846981 at `0x24D2448` (.rdata)

Bytes: `80 34 62 40 01 00 00 00 30 63 62 40 01 00 00 00 50 63 62 40 01 00 00 00 10 5f 62 40 01 00 00 00 80 63 62 40 01 00 00 00 30 39 62 40 01 00 00 00`

Slot 0: bytes `80 34 62 40 01 00 00 00`; pointer VA `0x140623480`; candidate target RVA `0x623480` (.text).
```asm
00623480  4c 89 4c 24 20                   mov qword ptr [rsp + 0x20], r9
00623485  48 89 4c 24 08                   mov qword ptr [rsp + 8], rcx
0062348A  55                               push rbp
0062348B  41 54                            push r12
0062348D  41 56                            push r14
0062348F  48 83 ec 70                      sub rsp, 0x70
00623493  0f b6 41 12                      movzx eax, byte ptr [rcx + 0x12]
00623497  49 8b e9                         mov rbp, r9
0062349A  44 8b f2                         mov r14d, edx
0062349D  4d 8b e0                         mov r12, r8
006234A0  3b d0                            cmp edx, eax
006234A2  74 0c                            je 0x6234b0
006234A4  32 c0                            xor al, al
006234A6  48 83 c4 70                      add rsp, 0x70
006234AA  41 5e                            pop r14
006234AC  41 5c                            pop r12
006234AE  5d                               pop rbp
006234AF  c3                               ret 
006234B0  48 89 9c 24 98 00 00 00          mov qword ptr [rsp + 0x98], rbx
006234B8  48 8d 4c 24 48                   lea rcx, [rsp + 0x48]
```

Slot 7: bytes `70 3a 62 40 01 00 00 00`; pointer VA `0x140623A70`; candidate target RVA `0x623a70` (.text).
```asm
00623A70  48 89 5c 24 10                   mov qword ptr [rsp + 0x10], rbx
00623A75  55                               push rbp
00623A76  57                               push rdi
00623A77  41 54                            push r12
00623A79  41 56                            push r14
00623A7B  41 57                            push r15
00623A7D  48 83 ec 20                      sub rsp, 0x20
00623A81  4c 8b f2                         mov r14, rdx
00623A84  c6 44 24 60 00                   mov byte ptr [rsp + 0x60], 0
00623A89  4c 8b f9                         mov r15, rcx
00623A8C  48 89 74 24 50                   mov qword ptr [rsp + 0x50], rsi
00623A91  40 b5 01                         mov bpl, 1
00623A94  49 c7 c4 ff ff ff ff             mov r12, 0xffffffffffffffff
00623A9B  0f 1f 44 00 00                   nop dword ptr [rax + rax]
00623AA0  40 84 ed                         test bpl, bpl
00623AA3  0f 84 11 02 00 00                je 0x623cba
00623AA9  49 8b ce                         mov rcx, r14
00623AAC  e8 2f 4e cd ff                   call 0x2f88e0
00623AB1  85 c0                            test eax, eax
00623AB3  0f 84 01 02 00 00                je 0x623cba
```

#### ID 859152 at `0x24D06F8` (.rdata)

Bytes: `20 a5 61 40 01 00 00 00 70 a7 61 40 01 00 00 00 a0 a2 61 40 01 00 00 00 c0 98 61 40 01 00 00 00 50 a7 61 40 01 00 00 00 40 a3 61 40 01 00 00 00`

Slot 0: bytes `20 a5 61 40 01 00 00 00`; pointer VA `0x14061A520`; candidate target RVA `0x61a520` (.text).
```asm
0061A520  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
0061A525  57                               push rdi
0061A526  48 83 ec 20                      sub rsp, 0x20
0061A52A  48 8d 05 c7 61 eb 01             lea rax, [rip + 0x1eb61c7]
0061A531  48 8b f9                         mov rdi, rcx
0061A534  48 89 01                         mov qword ptr [rcx], rax
0061A537  8b da                            mov ebx, edx
0061A539  48 83 c1 18                      add rcx, 0x18
0061A53D  e8 be 27 06 01                   call 0x167cd00
0061A542  48 8d 05 ef 5f eb 01             lea rax, [rip + 0x1eb5fef]
0061A549  48 89 07                         mov qword ptr [rdi], rax
0061A54C  f6 c3 01                         test bl, 1
0061A54F  74 0d                            je 0x61a55e
0061A551  ba 30 00 00 00                   mov edx, 0x30
0061A556  48 8b cf                         mov rcx, rdi
0061A559  e8 7a 41 ca 01                   call 0x22be6d8
0061A55E  48 8b 5c 24 30                   mov rbx, qword ptr [rsp + 0x30]
0061A563  48 8b c7                         mov rax, rdi
0061A566  48 83 c4 20                      add rsp, 0x20
0061A56A  5f                               pop rdi
```

Slot 5: bytes `40 a3 61 40 01 00 00 00`; pointer VA `0x14061A340`; candidate target RVA `0x61a340` (.text).
```asm
0061A340  48 89 5c 24 08                   mov qword ptr [rsp + 8], rbx
0061A345  48 89 74 24 10                   mov qword ptr [rsp + 0x10], rsi
0061A34A  57                               push rdi
0061A34B  48 83 ec 20                      sub rsp, 0x20
0061A34F  48 8b f9                         mov rdi, rcx
0061A352  48 8b da                         mov rbx, rdx
0061A355  48 8b ca                         mov rcx, rdx
0061A358  40 b6 01                         mov sil, 1
0061A35B  e8 80 e5 cd ff                   call 0x2f88e0
0061A360  3d 45 50 46 32                   cmp eax, 0x32465045
0061A365  0f 84 10 01 00 00                je 0x61a47b
0061A36B  3d 45 50 46 33                   cmp eax, 0x33465045
0061A370  74 53                            je 0x61a3c5
0061A372  3d 45 50 46 44                   cmp eax, 0x44465045
0061A377  0f 85 0a 01 00 00                jne 0x61a487
0061A37D  83 bb b8 02 00 00 04             cmp dword ptr [rbx + 0x2b8], 4
0061A384  40 0f 94 c6                      sete sil
0061A388  0f 85 f9 00 00 00                jne 0x61a487
0061A38E  41 b8 04 00 00 00                mov r8d, 4
0061A394  c7 44 24 40 00 00 00 00          mov dword ptr [rsp + 0x40], 0
```

#### Dialogue.cpp pattern

Pattern `48 83 C6 18 48 83 C7 18`; file-backed .text hit count **1**. Entire min(VirtualSize, RawSize) searched. Protected on-disk zero hits do not establish loaded-image absence.

Hit `0x2157F2F`; +0xC instruction `0x2157F3B` bytes `49 8b b6 88 00 00 00`; signed disp32 136 (`0x00000088`); operand RVA `0x2157FCA`; operand - 0x28 = map candidate RVA `0x2157FA2` (.text). Container ABI UNVERIFIED.
```asm
02157F1F  00 00                            add byte ptr [rax], al
02157F21  00 e8                            add al, ch
02157F23  e9 43 52 ff 48                   jmp 0x4b14d16b
02157F28  8b ce                            mov ecx, esi
02157F2A  e8 e1 43 52 ff                   call 0x167c310
02157F2F  48 83 c6 18                      add rsi, 0x18
02157F33  48 83 c7 18                      add rdi, 0x18
02157F37  85 ed                            test ebp, ebp
02157F39  75 c8                            jne 0x2157f03
02157F3B  49 8b b6 88 00 00 00             mov rsi, qword ptr [r14 + 0x88]
02157F42  49 8b 8e 80 00 00 00             mov rcx, qword ptr [r14 + 0x80]
02157F49  48 8b d6                         mov rdx, rsi
02157F4C  e8 bf 48 50 ff                   call 0x165c810
02157F51  33 c0                            xor eax, eax
02157F53  49 89 86 88 00 00 00             mov qword ptr [r14 + 0x88], rax
02157F5A  eb 02                            jmp 0x2157f5e
02157F5C  33 c0                            xor eax, eax
02157F5E  49 c7 46 6c 00 00 00 00          mov qword ptr [r14 + 0x6c], 0
02157F66  49 8d 4e 38                      lea rcx, [r14 + 0x38]
02157F6A  41 89 46 74                      mov dword ptr [r14 + 0x74], eax
02157F6E  e8 9d c2 fb ff                   call 0x2114210
02157F73  49 8d 4e 08                      lea rcx, [r14 + 8]
02157F77  e8 f4 c1 fb ff                   call 0x2114170
```

## Shared wrapper ID bin lookups

Source roles/pairs from `docs/runtime-inventory.md` section 5.1. Both members of each header-declared OG/NG pair are looked up independently in EVERY bin. These are numeric observations, not semantic certification. RTTI descriptor IDs are outside this extension; the singleton engine cast ID is included. No sparse-map -0x08 adjustment has been applied to the raw bin RVA.

| ID | Inventory role | 1-10-163-0 | 1-10-980-0 | 1-10-984-0 | 1-11-137-0 | 1-11-159-0 | 1-11-169-0 | 1-11-191-0 | 1-11-221-0 | 1-11-240-0 |
|---:|---|---|---|---|---|---|---|---|---|---|
| 422985 | All form lookup: global form map / read lock | 0x58D36C8 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2689178 | All form lookup: global form map / read lock | MISSING | 0x2E68988 | 0x2E69988 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 691815 | All form lookup: global form map / read lock | 0x58D4130 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2689189 | All form lookup: global form map / read lock | MISSING | 0x2E689D8 | 0x2E699D8 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 1309967 | All wrong-type diagnostic strings: enum-string array | 0x36D67D0 | 0x2C69880 | 0x2C6A880 | 0x2EBFB60 | 0x2EC0B60 | 0x2EC5B60 | 0x2ED0B60 | 0x2ED0B60 | 0x2EDBB60 |
| 1376557 | Body/quest file ownership | 0x153690 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2193103 | Body/quest file ownership | MISSING | 0x2BDC60 | 0x2BDC90 | 0x30F140 | 0x30F280 | 0x30F670 | 0x3120A0 | 0x3120B0 | 0x3123D0 |
| 711558 | TESDataHandler singleton | 0x58CF080 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2688883 | TESDataHandler singleton | MISSING | 0x2E63E68 | 0x2E64E68 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 226372 | FULL sparse map | 0x36D7890 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2661402 | FULL sparse map | MISSING | 0x2C6A820 | 0x2C6B820 | 0x2EC0B00 | 0x2EC1B00 | 0x2EC6B00 | 0x2ED1B00 | 0x2ED1B00 | 0x2EDCB00 |
| 8308 | GMST singleton | 0x58E1030 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2690301 | GMST singleton | MISSING | 0x2E753E0 | 0x2E763E0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 84112 | NPC copy runtime cast | 0x2936C62 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 303410 | Player singleton | 0x5AA4388 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2690919 | Player singleton | MISSING | 0x2F6A8F0 | 0x2F6B8F0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 457340 | Instance name rebuild | 0x89DC0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2190179 | Instance name rebuild | MISSING | 0x22A2A0 | 0x22A2D0 | 0x27B680 | 0x27B7C0 | 0x27BBB0 | 0x27E5E0 | 0x27E5E0 | 0x27E900 |
| 862142 | Cell event singleton | 0x10B740 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2192250 | Cell event singleton | MISSING | 0x289F00 | 0x289F30 | 0x2DABC0 | 0x2DAD00 | 0x2DB0F0 | 0x2DDB20 | 0x2DDB20 | 0x2DDE40 |
| 407804 | BSStringT<char>.Set (wheel/descriptions/journal) | 0x44040 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2189084 | BSStringT<char>.Set (wheel/descriptions/journal) | MISSING | 0x1F7960 | 0x1F79C0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 507142 | Narrow string-pool GetEntry / Entry.release | 0x1B43DB0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2268729 | Narrow string-pool GetEntry / Entry.release | MISSING | 0x1563240 | 0x1563610 | 0x1678390 | 0x1678A60 | 0x16793E0 | 0x167D900 | 0x167DA00 | 0x167DD20 |
| 1204430 | Narrow string-pool GetEntry / Entry.release | 0x1B42FD0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2268720 | Narrow string-pool GetEntry / Entry.release | MISSING | 0x15629C0 | 0x1562D90 | 0x1677B10 | 0x16781E0 | 0x1678B60 | 0x167D080 | 0x167D180 | 0x167D4A0 |
| 343176 | Game heap singleton / Allocate / Deallocate | 0x48A0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2193197 | Game heap singleton / Allocate / Deallocate | MISSING | 0x2C0C20 | 0x2C0C50 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 652767 | Game heap singleton / Allocate / Deallocate | 0x1B0EFD0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2267872 | Game heap singleton / Allocate / Deallocate | MISSING | 0x153D400 | 0x153D7D0 | 0x1652450 | 0x1652B20 | 0x16534A0 | 0x16579C0 | 0x1657AC0 | 0x1657DE0 |
| 1582181 | Game heap singleton / Allocate / Deallocate | 0x1B0F2E0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2267874 | Game heap singleton / Allocate / Deallocate | MISSING | 0x153D860 | 0x153DC30 | 0x16528B0 | 0x1652F80 | 0x1653900 | 0x1657E20 | 0x1657F20 | 0x1658240 |
| 1425657 | Cell spin-lock acquisition | 0x244D0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2192245 | Cell spin-lock acquisition | MISSING | 0x2898C0 | 0x2898F0 | 0x2DA580 | 0x2DA6C0 | 0x2DAAB0 | 0x2DD4E0 | 0x2DD4E0 | 0x2DD800 |
| 1573164 | Read/write lock acquisition | 0x1B10930 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2267897 | Read/write lock acquisition | MISSING | 0x153EA20 | 0x153EDF0 | 0x1653A70 | 0x1654140 | 0x1654AC0 | 0x1658FE0 | 0x16590E0 | 0x1659400 |
| 336186 | Read/write lock acquisition | 0x1B109B0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2267898 | Read/write lock acquisition | MISSING | 0x153EAA0 | 0x153EE70 | 0x1653AF0 | 0x16541C0 | 0x1654B40 | 0x1659060 | 0x1659160 | 0x1659480 |
| 244786 | GFx managed Value add/release | 0x20B9C30 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2286228 | GFx managed Value add/release | MISSING | 0x19BB410 | 0x19BB7E0 | 0x1AD0DE0 | 0x1AD14D0 | 0x1AD1E50 | 0x1AD6590 | 0x1AD66B0 | 0x1AD6B70 |
| 856221 | GFx managed Value add/release | 0x20B9C80 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2286229 | GFx managed Value add/release | MISSING | 0x19BB450 | 0x19BB820 | 0x1AD0E20 | 0x1AD1510 | 0x1AD1E90 | 0x1AD65D0 | 0x1AD66F0 | 0x1AD6BB0 |
| 254218 | GFx GetArraySize / GetMember | 0x20A1E60 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2285791 | GFx GetArraySize / GetMember | MISSING | 0x19A9300 | 0x19A96D0 | 0x1ABECD0 | 0x1ABF3C0 | 0x1ABFD40 | 0x1AC4480 | 0x1AC45A0 | 0x1AC4A60 |
| 1517430 | GFx GetArraySize / GetMember | 0x20A8060 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2285936 | GFx GetArraySize / GetMember | MISSING | 0x19AD380 | 0x19AD750 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 1360149 | GFx SetMember / Invoke | 0x20D05E0 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2286589 | GFx SetMember / Invoke | MISSING | 0x19CB820 | 0x19CBBF0 | 0x1AE1170 | 0x1AE1860 | 0x1AE21E0 | 0x1AE6920 | 0x1AE6A40 | 0x1AE6F00 |
| 655847 | GFx SetMember / Invoke | 0x20B1D20 | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING | MISSING |
| 2286101 | GFx SetMember / Invoke | MISSING | 0x19B4B40 | 0x19B4F10 | 0x1ACA510 | 0x1ACAC00 | 0x1ACB580 | 0x1ACFCC0 | 0x1ACFDE0 | 0x1AD02A0 |

## Key findings and remaining proof

1. **OG disk .text is protected/encrypted-looking**: entropy 8.00000 bits/byte, arbitrary bytes at all 12 function IDs, and zero prompt-pattern hits. OG vtable pointers are readable but their target code cannot be certified from this disk image. This is not proof of a particular encryption algorithm.
2. **AE .191/.221/.240 code is readable** (.text entropy about 6.4746). Each has one exact prompt-pattern hit, but match+0xC is `49 8B B6 88 00 00 00` = `mov rsi, qword ptr [r14 + 0x88]`, NOT RIP-relative. Treating +3 as RIP disp32 produces a bogus table candidate inside .text. Uniqueness alone is insufficient; the existing scan recipe is incompatible with these AE hits.
3. All 12 explicit project function OG IDs are absent from every NG/AE bin examined. The five vtable IDs (including contextual primary TESLevItem) exist in all nine bins. Vtable numeric continuity is not proof of ABI/layout continuity.
4. Every NG/AE function signature, structure/member offset, calling convention and capability remains UNVERIFIED. NG .980/.984 and AE .137/.159/.169 have no executable supplied. No counterpart guessed.
5. Layouts not independently proven here include QuestObjective 0x28; action +0x08; TESLevItem base +0x68 / slot 0x0A; AVIF +0x160 vs +0x1A0; GFx private Value and Params; perk data/label +0x18/index +0x28; region name +0x10; all accessed CommonLib members and container/ownership ABIs. Entry-point bytes and addresses do not prove these.
6. **In-game: UNVERIFIED for every row.** No build or tests executed by this evidence investigation. Keep NG/AE feature paths disabled until semantic and layout proof exists.
