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

## Credits

- SkyHorizon: original SSE Dynamic String Distributor. See its [mod page](https://www.nexusmods.com/skyrimspecialedition/mods/107676) for the original credits.
- alandtse and contributors: CommonLibF4.
- Agent69-stack: [FallHook](https://github.com/Agent69-stack/FallHook) (GPL-3.0). The OG 1.10.163 hooks for dialogue options, crosshair prompts, quest journal text and some text fields, and the prologue hook helper, are adapted from it.

## License

GPL-3.0, same as the original SSE Dynamic String Distributor. Original work © SkyHorizon.
Uses [CommonLibF4](https://github.com/alandtse/CommonLibF4) (MIT).
