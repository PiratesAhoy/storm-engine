# Dependency map

This document is the fast-recall map for how Storm Engine dependencies fit together. It is grounded in the top-level `CMakeLists.txt`, `conanfile.py`, `cmake/StormSetup.cmake`, and the `src/libs/*/CMakeLists.txt` target declarations.

For broader context, see `docs/project-structure.md` and `docs/architecture.md`. For the renderer migration plan, see `docs/plans/renderer-backend-abstraction-plan.md`.

## One-screen summary

- Build entry point: `CMakeLists.txt` configures Conan, global options, platform D3D9 compatibility, external packages, render technique copying, then adds `src/`.
- Target helper: `STORM_SETUP(...)` in `cmake/StormSetup.cmake` creates executables, normal libraries, and `storm_module` static libraries from conventional `include/`, `src/`, `testsuite/`, and `rsrc/` folders.
- Runtime composition: `src/apps/engine` links the small `engine` executable against base libraries and all registered `storm_module` targets. Non-Windows executable linking uses whole-archive semantics for modules so macro/static registration is retained.
- Architectural hub: `core` is the dominant internal hub. Almost every module depends on it for entities, services, script functions, layers, attributes, and runtime lifecycle.
- Portability bottleneck: `renderer` is the second hub and is still D3D9-shaped. Many gameplay/UI modules depend directly on `VDX9RENDER`, so the renderer public interface is the key dependency seam for OpenGL/WebGPU migration.
- Rendering support spine: `renderer`, `geometry`, `model`, `animation`, `collide`, `sea`, `sea_ai`, `ship`, and `weather` form the main gameplay/rendering dependency cluster.
- Platform layer: `window` and `input` depend on SDL. Non-Windows rendering still depends on D3D9-compatible libraries from DXVK Native or Gallium Nine rather than a native OpenGL backend.
- External dependencies are declared in Conan. Windows-only packages include DirectX, FMOD, and 7zip; non-Windows CMake creates empty `directx::directx` and `fmod::fmod` interface targets where needed.

## Build and package flow

1. `CMakeLists.txt` prepends `cmake/conan_provider.cmake` to `CMAKE_PROJECT_TOP_LEVEL_INCLUDES`.
2. Project options are normalized and passed into Conan as root recipe options:
   - `STORM_ENABLE_CRASH_REPORTS`
   - `STORM_ENABLE_STEAM`
   - `STORM_USE_CONAN_SDL`
   - generated output paths such as `STORM_WATERMARK_FILE`
3. On non-Windows, `cmake/linux.cmake` selects the D3D9 compatibility layer:
   - `STORM_MESA_NINE=ON`: Gallium Nine / `nine-native`
   - default: DXVK Native external project
4. `find_package(...)` resolves Conan packages and platform placeholder targets.
5. `engine_techniques` copies `src/techniques/**/*.fx` into `bin/resource/techniques`.
6. `src/CMakeLists.txt` adds `src/libs` before `src/apps`, which is important because apps link against the globally registered module list.
7. `STORM_SETUP(TYPE storm_module)` appends modules to `global_modules_list`; `STORM_SETUP(TYPE executable)` appends that module list to executable dependencies and applies whole-archive linker flags.

## Conan package roles

| Package/target | Scope | Main consumers | Role |
| --- | --- | --- | --- |
| `zlib/1.3.1` / `zlib` | all platforms | `engine` | Compression/runtime support. |
| `spdlog/1.13.0` / `spdlog::spdlog` | all platforms | `diagnostics` | Logging backend. |
| `fast_float/8.0.2` / `FastFloat::fast_float` | all platforms | `core` | Fast numeric parsing. |
| `mimalloc/2.1.7` / `mimalloc` | all platforms | `engine`, deploy hooks | Allocator/runtime binary deployment. |
| `sentry-native/0.6.5` / `sentry::sentry` | all platforms | `engine`, `diagnostics` | Crash diagnostics and crashpad deployment. |
| `tomlplusplus/3.3.0` / `tomlplusplus::tomlplusplus` | all platforms | `core` | TOML config parsing. |
| `nlohmann_json/3.11.2` / `nlohmann_json::nlohmann_json` | all platforms | `core` | JSON config/data parsing. |
| `imgui/1.90-docking` / `imgui::imgui` | all platforms | `editor` via `imgui_backend` | In-engine developer UI. Conan also copies backend sources into the build tree. |
| `cli11/2.3.2` / `CLI11::CLI11` | all platforms | `engine` | Command-line parsing in the executable. |
| `ms-gsl/4.1.0` / `Microsoft.GSL::GSL` | all platforms | `sea_ai` | GSL helper types/contracts. |
| `sdl/2.32.2` / `${SDL2_LIBRARIES}` | optional Conan SDL | `engine`, `core`, `window`, `input`, `editor` backend | Window/events/input and ImGui SDL binding. |
| `catch2/3.9.1` / `Catch2::Catch2WithMain` | test requirement | `config-test`, `util-test`, `xinterface-test` | Unit test main/framework. |
| `directx/jun10+9.29.1962.1` / `directx::directx` | Windows only | `renderer` | DirectX 9 headers/libraries. Non-Windows has an interface placeholder target. |
| `fmod/2.02.05+1` / `fmod::fmod` | Windows only | `sound_service` | Audio backend and runtime DLL deployment. Non-Windows has an interface placeholder target. |
| `7zip/19.00` | Windows + crash reports | deploy hooks | Ships `7za.exe` when crash reports are enabled. |
| `steamworks/1.5.1` | optional `STORM_ENABLE_STEAM` | `steam_api` | Steam integration and runtime DLL deployment. |

Transitive packages observed during Conan generation include `fmt`, `libcurl`, and `openssl`; these are pulled by direct packages such as `spdlog` and `sentry-native`, not linked directly by Storm targets.

## Internal target categories

### Base/runtime libraries

| Target | Type | Depends on | Purpose |
| --- | --- | --- | --- |
| `core` | library | `diagnostics`, `editor`, `math`, `shared_headers`, `sound`, `steam_api`, `FastFloat`, SDL, `window`, TOML, JSON | Runtime API: entities, services, attributes, scripts, layers, events, save/load, window access. This creates several compile/link cycles with modules that register services/entities. |
| `diagnostics` | library | `core`, `util`, `spdlog`, `sentry` | Logging/crash diagnostics. |
| `math` | library | none | Engine math types. Some current headers still include D3D9 types and should be sanitized during renderer abstraction. |
| `util` | library | test target uses Catch2 | Cross-cutting helpers, platform compatibility, D3DX compatibility declarations, handles. |
| `window` | library | SDL | SDL-backed OS window abstraction. |
| `input` | library | SDL, `util` | SDL/input handling support used by controls. |
| `shared_headers` | library | `core` | Shared script/resource constants copied to runtime resources. |

### Platform, renderer, and developer UI

| Target | Type | Depends on | Purpose |
| --- | --- | --- | --- |
| `renderer` | storm_module | `core`, `config`, `directx::directx`, `util`, platform D3D9/system libs | D3D9-shaped renderer service (`VDX9RENDER`/`DX9RENDER`), device lifecycle, resources, states, techniques, draw paths. Main portability bottleneck. |
| `editor` | storm_module | `core`, plus `imgui_backend` when ImGui is found | In-engine ImGui tools. The backend links ImGui, SDL, and on Windows the DX9 ImGui backend; non-Windows links native D3D9 compatibility libs. |
| `config` | storm_module | `core`; test uses Catch2 | Engine configuration support and script-visible config library. |
| `steam_api` | storm_module | `core`, optional Steamworks | Optional Steam achievements/API integration. |

### Content and rendering services

| Target | Type | Depends on | Purpose |
| --- | --- | --- | --- |
| `geometry` | storm_module | `core`, `renderer` | Geometry loading/creation/rendering helpers; a major service consumed by many modules. |
| `model` | storm_module | `animation`, `collide`, `core`, `geometry`, `renderer` | Model loading/rendering and model-related runtime objects. |
| `animation` | storm_module | `core`, `util` | Animation service and action/bone/player types. |
| `collide` | storm_module | `core` | Collision service and local collision objects. |
| `particles` | storm_module | `core`, `geometry`, `renderer`, `util` | Particle service and processors. |
| `lighter` | storm_module | `core`, `geometry`, `model`, `renderer`, `util` | Lighting-related systems/tools. |
| `shadow` | storm_module | `collide`, `core`, `geometry`, `model`, `renderer` | Shadow entity/system. |
| `locator` | storm_module | `core`, `renderer`, `sea_ai` | Locator/blast helpers. |

### Sea/gameplay cluster

| Target | Type | Depends on | Purpose |
| --- | --- | --- | --- |
| `sea` | storm_module | `core`, `renderer`, `sea_ai` | Sea rendering/simulation base. |
| `sea_ai` | storm_module | `collide`, `core`, `geometry`, `island`, `location`, `model`, `particles`, `renderer`, `sea`, `ship`, GSL | Ship/fort/ball AI and sea simulation orchestration; highly coupled. |
| `ship` | storm_module | `collide`, `core`, `geometry`, `island`, `location`, `model`, `sea`, `sea_ai`, `particles`, `renderer` | Ship entity/systems. |
| `island` | storm_module | `collide`, `core`, `geometry`, `model`, `renderer`, `sea`, `sea_ai`, `weather` | Island systems and rendering. |
| `weather` | storm_module | `collide`, `core`, `geometry`, `renderer`, `sea`, `ship` | Weather, sky, rain, astronomy, flares. |
| `battle_interface` | storm_module | `animation`, `core`, `geometry`, `island`, `model`, `particles`, `renderer`, `sea_ai`, `ship`, `weather` | Sea/land battle UI and indicators. |
| `worldmap` | storm_module | `battle_interface`, `core`, `geometry`, `location`, `renderer`, `util` | World map runtime. |

### Location/interface/audio and smaller modules

| Target | Type | Depends on | Purpose |
| --- | --- | --- | --- |
| `location` | storm_module | `animation`, `blade`, `collide`, `core`, `geometry`, `model`, `renderer`, `sea`, `sound_service`, `util` | Land/location entities, characters, cameras, grass, fader, effects. |
| `xinterface` | storm_module | `animation`, `core`, `geometry`, `model`, `renderer`, `util`, Windows media/system libs; test uses Catch2 | Legacy UI/interface system, string service, interface nodes, video/screenshots. |
| `dialog` | storm_module | `core`, `geometry`, `model`, `renderer`, `sound_service` | Dialog UI/runtime. |
| `sound_service` | storm_module | `core`, `fmod::fmod`, `renderer`, `editor` | Sound playback/mixing service. Still depends on renderer/editor for debug/UI/runtime integration. |
| `sound` | storm_module | `core`, `renderer`, `sound_service` | Script/entity-facing sound integration. |
| `pcs_controls` | storm_module | `core`, `input` | Control mapping/state service. |
| `script_library` | storm_module | `core` | Script-visible helper library. |
| `animals`, `ball_splash`, `blade`, `blot`, `mast`, `rigging`, `sailors`, `sea_cameras`, `sea_creatures`, `sea_foam`, `sea_operator`, `sink_effect`, `teleport`, `tornado`, `touch`, `water_rings` | storm_module | Mostly combinations of `core`, `renderer`, `geometry`, `model`, `sea`, `sea_ai`, `ship`, `collide`, `sound_service` | Leaf or near-leaf gameplay/effects modules. Most retrieve renderer/geometry services at runtime and are likely renderer-abstraction call sites. |

## Most depended-on internal targets

Generated from `src/libs/*/CMakeLists.txt` target dependencies:

| Target | Number of direct dependents | Why it matters |
| --- | ---: | --- |
| `core` | 44 | Runtime/service/entity/script hub. Changes ripple everywhere. |
| `renderer` | 33 | Rendering API hub and D3D9 leak surface. Primary migration seam. |
| `geometry` | 28 | Shared geometry/model rendering support. Often sits between gameplay and renderer. |
| `model` | 20 | Shared model runtime used by location/sea/UI/effects. |
| `sea` | 15 | Central sea simulation/rendering dependency. |
| `collide` | 13 | Collision queries used by gameplay/rendering systems. |
| `ship` | 13 | Sea gameplay hub. |
| `util` | 9 | Cross-cutting helpers and compatibility. |
| `animation` | 8 | Character/model/action animation support. |
| `sea_ai` | 8 | Highly coupled sea gameplay orchestration. |
| `island` | 7 | Island/location/sea integration point. |
| `sound_service` | 7 | Audio service consumed by gameplay/effects. |

## Runtime dependency pattern

Compile/link dependencies do not tell the whole story because runtime lookup happens through `Core`:

- Services are registered with `CREATE_SERVICE(...)` in module source files.
- Entities/classes are registered with `CREATE_CLASS(...)`.
- Script libraries are registered with `CREATE_SCRIPTLIBRIARY(...)`.
- Callers usually retrieve services with `core.GetServiceX<T>()`.

Important registered services include:

| Service/interface | Module | Notes |
| --- | --- | --- |
| `DX9RENDER` / `VDX9RENDER` | `renderer` | Main renderer service. Many modules call `core.GetServiceX<VDX9RENDER>()`. |
| `LostDeviceSentinel` | `renderer` | Watches D3D9 device loss/reset and pauses frame processing. |
| `GEOMETRY` / `VGEOMETRY` | `geometry` | Geometry service frequently used alongside renderer. |
| `AnimationServiceImp` / `AnimationService` | `animation` | Animation object service. |
| `COLL` / `COLLIDE` | `collide` | Collision service. |
| `ParticleService` / `IParticleService` | `particles` | Particle system service. |
| `SoundService` / `VSoundService` | `sound_service` | Audio service. |
| `PCS_CONTROLS` / `CONTROLS` | `pcs_controls` | Input/control state service. |
| `STRSERVICE` / string service | `xinterface` | Localization/string lookup service. |

## Renderer migration dependency seams

When changing renderer architecture, remember this dependency order:

1. `renderer/include/dx9render.h` is the public contract most callers compile against.
2. `renderer/src/s_device.h` and `renderer/src/s_device.cpp` are the concrete D3D9 backend/device implementation.
3. Many modules link `renderer` but only need higher-level operations such as texture lookup, draw calls, font output, render states, or matrices.
4. `geometry`, `model`, `particles`, `sea`, `weather`, `location`, `battle_interface`, `xinterface`, and `worldmap` are the highest-value caller groups for replacing D3D9 names with neutral renderer types.
5. `editor` and `configapp` are separate ImGui/D3D9 surfaces; they can migrate to SDL/OpenGL adapters independently from the game renderer.
6. `math` and shared utility headers should not include D3D9 types if they are meant to be backend-neutral.

Practical rule: do not start by replacing `s_device.cpp`. First introduce neutral types/handles/adapters in the public renderer boundary, convert callers in clusters, then split frontend service/resource handles from backend implementation.

## Current non-Windows caveats

- Non-Windows still builds against a D3D9 API compatibility layer; it is not a native OpenGL path.
- `cmake/linux.cmake` currently names the non-Windows path even on macOS and selects DXVK Native by default.
- The external D3D9 compatibility project must provide headers/libraries before renderer compilation can succeed.
- `directx::directx` and `fmod::fmod` are interface placeholders on non-Windows so unrelated modules can configure while Windows-only packages stay scoped to Windows.

## How to refresh this map

Useful commands from the repository root:

```sh
# Reconfigure and emit a full target graph in the existing build directory.
cmake --graphviz=build-blaze/dependencies.dot build-blaze

# Print target/type/dependency rows from STORM_SETUP declarations.
python3 - <<'PY'
import pathlib, re
root = pathlib.Path('.')
print('target|type|dependencies|test_dependencies|path')
for p in sorted(root.glob('src/libs/*/CMakeLists.txt')):
    txt = p.read_text(errors='ignore')
    m = re.search(r'STORM_SETUP\s*\((.*?)\)', txt, re.S)
    if not m:
        continue
    body = '\n'.join(line.split('#', 1)[0] for line in m.group(1).splitlines())
    target = re.search(r'TARGET_NAME\s+([^\s]+)', body).group(1)
    typ = re.search(r'TYPE\s+([^\s]+)', body).group(1)
    def section(name):
        mm = re.search(r'(^|\s)' + name + r'\s+(.*?)(?:\s+TEST_DEPENDENCIES|\s+LINKER_FLAGS|$)', body, re.S)
        return ', '.join(x for x in re.split(r'\s+', mm.group(2).strip()) if x) if mm else '-'
    tdeps = re.search(r'(^|\s)TEST_DEPENDENCIES\s+(.*?)(?:\s+LINKER_FLAGS|$)', body, re.S)
    tdeps = ', '.join(x for x in re.split(r'\s+', tdeps.group(2).strip()) if x) if tdeps else '-'
    print(f'{target}|{typ}|{section("DEPENDENCIES")}|{tdeps}|{p}')
PY
```

Do not commit generated `build-blaze/dependencies.dot` unless the project intentionally starts tracking generated dependency graphs.
