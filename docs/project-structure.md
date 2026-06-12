# Project structure

This document is a quick map of the repository for contributors. It describes where the main build files, runtime code, engine modules, tools, and documentation live.

## Top-level layout

- `CMakeLists.txt` - top-level CMake entry point. It configures global options, Conan integration, dependency lookup, shader/effect copying, and then includes `src/`.
- `conanfile.py` - Conan dependency recipe and resource deployment hooks. It installs runtime assets generated from source-controlled data, copies ImGui backends into the build tree, and deploys required DLLs/shared libraries.
- `cmake/` - CMake helper modules.
  - `StormSetup.cmake` defines the `STORM_SETUP(...)` macro used by most engine targets.
  - `linux.cmake` configures the non-Windows D3D9 compatibility layer through Gallium Nine or DXVK Native.
  - `conan_provider.cmake` integrates Conan with CMake package discovery.
- `src/` - all engine source code, split into applications, modules, shared headers, and render techniques.
- `src/techniques/` - shader/effect assets copied to `bin/resource/techniques` by the top-level build.
- `docs/` - architecture, services, scripting, tools, and legacy technical documentation.
- `tools/` - standalone import/export/conversion/editor utilities and older asset processing helpers.
- `rsrc/` - repository resources used by the build/runtime.

## Source tree

### `src/apps/`

Application entry points.

- `src/apps/engine/` - the main `engine` executable.
  - `src/apps/engine/src/main.cpp` initializes CLI options, SDL, diagnostics, logging, the engine core, the OS window, and the main frame loop.
  - `src/apps/engine/CMakeLists.txt` links the executable with the core runtime dependencies and all registered storm modules.
- `src/apps/configapp/` - standalone configuration UI.
  - `src/apps/configapp/src/configapp.cpp` owns its own SDL window, D3D9 device, and ImGui frame loop.

### `src/libs/`

Engine modules and internal libraries. The directory-level `src/libs/CMakeLists.txt` calls `add_all_subdirectories()`, so each module is configured by its own `CMakeLists.txt`.

Most modules use `STORM_SETUP(...)`:

- `TYPE storm_module` - static module linked into `engine` with whole-archive semantics so registration symbols are retained.
- `TYPE library` - normal support library.
- `TYPE executable` - application target.

Important modules:

- `core` - central runtime API, entity/service access, events, script integration, layers, save/load, and the global `core` object.
- `window` - SDL-backed OS window abstraction.
- `renderer` - current DirectX 9 renderer service (`VDX9RENDER`/`DX9RENDER`), texture/font/buffer management, render states, post-processing, and technique/effect execution.
- `editor` - in-engine ImGui developer tools integration.
- `config` - engine configuration support.
- `diagnostics` - lifecycle/crash diagnostics support.
- `math` - custom vector/matrix/math types.
- `util` - platform and utility helpers, including D3DX compatibility declarations for non-Windows builds.
- `geometry`, `model`, `animation`, `rigging` - model/geometry/animation loading, rendering, and rig-related systems.
- `collide`, `locator`, `lighter` - collision, locators, and lighting tools/systems.
- `particles`, `sea`, `weather`, `shadow`, `location`, `worldmap` - major game rendering and simulation systems.
- `xinterface`, `battle_interface`, `dialog` - UI/interface systems.
- `sound`, `sound_service` - sound playback and service integration.
- `input`, `pcs_controls`, `touch` - input and control handling.
- `script_library` - script-visible functions and script integration helpers.
- `steam_api` - optional Steam integration when enabled.
- `shared_headers` - shared constants/types/resources installed into the runtime resource tree.

### `src/techniques/`

Contains `.fx` technique/effect files and precompiled shader objects used by the renderer. The top-level `CMakeLists.txt` copies `src/techniques/**/*.fx` into the build output under `bin/resource/techniques` through the `engine_techniques` target.

The current renderer has two technique/effect paths:

- Windows uses D3DX effects (`src/libs/renderer/src/effects.*`).
- Non-Windows uses the custom technique parser (`src/libs/renderer/src/technique.*`) against a native D3D9-compatible backend.

## Build organization

The top-level build does the following:

1. Loads Conan through `cmake/conan_provider.cmake`.
2. Sets global C++ standard/options.
3. Defines project options such as crash reports, Steam, safe mode, SDL source, and Mesa Nine.
4. Configures Linux D3D9 compatibility if not building on Windows.
5. Finds external dependencies (`zlib`, `spdlog`, `mimalloc`, `sentry`, `tomlplusplus`, `nlohmann_json`, `cli11`, `directx`, `fmod`, `Microsoft.GSL`, `ImGui`, `SDL2`, `Catch2`).
6. Copies render technique files into the build output.
7. Adds `src/`.

`STORM_SETUP(...)` is the standard target helper. For executables, it appends all registered `storm_module` targets to the link line and forces module symbols to be preserved. This is important because module registration is macro/static-initializer driven.

## Runtime output/resource deployment

Conan generation (`conanfile.py`) copies runtime resources and third-party binaries into the selected build output directory. Notable generated/copied data includes:

- `src/techniques` -> `resource/techniques`
- `src/libs/shared_headers/include/shared` -> `resource/shared`
- ImGui backends from the Conan ImGui package into `${CMAKE_BINARY_DIR}/imgui`
- FMOD, mimalloc, sentry/crashpad, and optional Steam/7zip runtime binaries depending on OS/options

## Documentation map

- `docs/architecture.md` - high-level runtime architecture and rendering migration notes.
- `docs/renderer-backend-abstraction-plan.md` - staged plan for abstracting DirectX 9 behind a backend-neutral renderer API.
- `docs/coding-guidelines.md` - coding conventions inferred from historical changes.
- `docs/project-structure.md` - this file.
- `docs/services.md` - service registration and known service overview.
- `docs/core.txt`, `docs/syntax.txt`, `docs/subscribe.txt` - legacy scripting/core documentation.
- Tool-specific documentation lives either in `docs/` or the corresponding `tools/*/README.md`.
