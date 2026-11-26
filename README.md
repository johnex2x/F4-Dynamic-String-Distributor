# F4 Dynamic String Distributor

F4SE plugin that replaces game text at runtime from JSON translation files, without editing plugin files.
Fallout 4 port of [SSE Dynamic String Distributor](https://github.com/SkyHorizon3/SSE-Dynamic-String-Distributor) by SkyHorizon.

- Runtimes: Fallout 4 OG 1.10.163 and AE 1.11.x, one DLL for both, with the F4SE build for that runtime and Address Library for F4SE Plugins. NG 1.10.980/984 is not supported. AE status and evidence: `docs/runtime-support.md`.
- Translation files: `Data/F4SE/Plugins/DynamicStringDistributor/<Plugin>/*.json` (same format as SSE-DSD).
- Settings: `Data/F4SE/Plugins/DynamicStringDistributor.ini` (see `config/`).

## Build

Requirements: Visual Studio 2022 (or Build Tools) with the C++ workload, CMake 3.21+, vcpkg at `C:/vcpkg`.

```bat
git clone --recurse-submodules <this repo>
cmake --preset vs2022
cmake --build --preset release
```

Output: `build/Release/DynamicStringDistributor.dll` and the optional `build/plugins/MenuRefCountFix/Release/MenuRefCountFix.dll`.

## Changes from the original

- Ported to Fallout 4 with F4SE and CommonLibF4 (OG 1.10.163). The translation JSON format stays compatible with SSE-DSD.
- Adds Fallout 4 hooks: dialogue options (vanilla wheel and XDI), crosshair prompts, character creation text, leveled-list and template names.
- Runs on AE 1.11.x. CMake applies two small patches to CommonLibF4 (`cmake/patches/`): runtime detection, and an atomic Scaleform `AddRef`.
- `MenuRefCountFix.dll` (`plugins/MenuRefCountFix/`) is a separate, optional F4SE plugin. CommonLibF4's Scaleform `RefCountImpl::AddRef` increments the plain count instead of the atomic one, and plugins built with it inline that code. In two known builds (Upscaling 1.1.1, Addictol) this race caused a crash or hang after leaving a workbench on AE. The DLL patches only those exact builds (24 bytes must match) and leaves any other version alone. DynamicStringDistributor.dll itself does not touch other plugins.
- JSON `conditions` are read but not evaluated yet: an entry with conditions applies unconditionally. Fallout 4 additions: `rule` (INNR WNAM) and `choice` (PERK EPF2); INNR WNAM entries need `original`.
- Developed with the help of AI coding assistants (Claude and Codex).

## INNR WNAM entries

Instance naming rules (INNR) build names such as "Insulated Vault Suit" from parts. Each WNAM part is one entry:

```json
[
  {
    "form_id": "0023C063|Fallout4.esm",
    "type": "INNR WNAM",
    "original": "Insulated",
    "string": "絕緣"
  },
  {
    "form_id": "0023C063|Fallout4.esm",
    "type": "INNR WNAM",
    "original": "Treated",
    "string": "加工"
  }
]
```

- `original` is required. It is the rule's text in the game; an entry without it is skipped.
- Without `rule`, the entry matches every rule of that INNR whose text equals `original`. If no entry of that INNR matches, an entry with the same `original` from any INNR is used.
- `rule` is optional and rarely needed. Use it only when one INNR has the same text on several rules and those rules need different translations. `rule` is the rule's number in that INNR: count every WNAM from 1, through all rule sets in file order, empty ones included. It is not the same number as `index`. If a mod update moves the rules and that number now holds another text, the entry is not used.
- The text is read when the game loads, so the entry works whichever plugin's version of the INNR wins.

## Credits

- SkyHorizon: original SSE Dynamic String Distributor. See its [mod page](https://www.nexusmods.com/skyrimspecialedition/mods/107676) for the original credits.
- alandtse and contributors: CommonLibF4.
- Agent69-stack: [FallHook](https://github.com/Agent69-stack/FallHook) (GPL-3.0). The OG 1.10.163 hooks for dialogue options, crosshair prompts, quest journal text and some text fields, and the prologue hook helper, are adapted from it.

## License

GPL-3.0, same as the original SSE Dynamic String Distributor. Original work © SkyHorizon.
Uses [CommonLibF4](https://github.com/alandtse/CommonLibF4) (MIT).
