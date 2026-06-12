# Architecture overview

Storm Engine is a game engine organized around a small executable and many statically linked engine modules. Runtime systems are exposed through a global `Core` API, entities, services, script-visible functions, and layered frame processing.

This document summarizes the current architecture as it exists in the codebase. It is intended as a starting point for contributors and for larger refactors.

## Runtime entry point

The main executable is `src/apps/engine`.

`src/apps/engine/src/main.cpp` performs the top-level startup sequence:

1. Parse command line options with CLI11.
2. Initialize SDL event/video/game-controller subsystems.
3. Initialize diagnostics and logging.
4. Initialize the global engine core.
5. Create and attach the OS window.
6. Enter the frame loop.

The frame loop calls `CorePrivate::Run()` through `RunFrame()`. Window events update application state and may pause/resume sound through `VSoundService`.

## Core API

The public runtime API is `Core` in `src/libs/core/include/core.h`. The concrete implementation is hidden behind `CorePrivate` in `src/libs/core/include/core_private.h`.

The global object is:

```cpp
extern Core &core;
```

Major responsibilities exposed by `Core`:

- service lookup through `GetServiceX<T>()`
- entity creation, lookup, deletion, attributes, and message dispatch
- layer management for execute/realize processing
- event dispatch and posted events
- script function registration
- save/load state
- window and screen-size access
- editor access
- frame-processing control

`CorePrivate` adds lifecycle operations used by the executable:

- `Init()` / `CleanUp()`
- `Run()`
- `AppState(...)`
- window attachment
- editor enablement
- crash-info collection

## Entities and layers

Entities derive from `Entity` in `src/libs/core/include/entity.h`.

Each entity implements:

- `Init()`
- `ProcessStage(Entity::Stage stage, uint32_t delta = 0)`

Optional hooks include:

- `ProcessMessage(MESSAGE &msg)`
- `AttributeChanged(ATTRIBUTES *)`
- `ShowEditor()`

Entity stages are:

- `execute` - simulation/update work
- `realize` - rendering/realization work
- `lost_render` - render resources should be released/invalidated
- `restore_render` - render resources should be recreated/restored

Entities are assigned to layers with a priority. Core processes layer contents according to layer type and frozen state. Common script-visible layer functions are documented in the legacy core/syntax docs.

## Services

Services derive from `SERVICE` in `src/libs/core/include/service.h`.

A service may implement:

- `Init()`
- `RunStart()`
- `RunEnd()`
- `RunSection()`
- `LoadState(...)`
- `CreateState(...)`

Services are registered with the `CREATE_SERVICE(...)` macro. Public service interfaces usually define a static `ServiceName`, then callers retrieve them with:

```cpp
auto *renderer = core.GetServiceX<VDX9RENDER>();
```

See `docs/services.md` for the service registration convention and a growing service list.

## Module registration and linking

Most engine systems live under `src/libs/*` and are built through the `STORM_SETUP(...)` CMake macro in `cmake/StormSetup.cmake`.

Modules using `TYPE storm_module` are added to a global module list. When the main executable is linked, CMake appends those modules and uses whole-archive style linker flags so registration objects are retained even if no normal symbol reference points to them.

This is important for systems that register services, entities, or script libraries via macros/static initialization.

## Rendering architecture: current state

The current renderer is DirectX 9 shaped throughout its public API.

Important files:

- `src/libs/renderer/include/dx9render.h` - public renderer service interface (`VDX9RENDER`).
- `src/libs/renderer/src/s_device.h` - concrete renderer class declaration (`DX9RENDER`) and D3D9-owned resources.
- `src/libs/renderer/src/s_device.cpp` - device lifecycle, render states, resources, draw paths, post-processing, script-facing functions, and presentation.
- `src/libs/renderer/src/effects.*` - Windows D3DX effect handling.
- `src/libs/renderer/src/technique.*` - non-Windows custom technique parser for the D3D9-compatible path.
- `src/techniques/` - effect/technique assets copied into the runtime resource tree.

The renderer service currently exposes many D3D9 types directly, including device objects, surfaces, textures, buffers, shaders, matrices, formats, primitive types, render states, sampler states, and viewport/caps structures. Many gameplay/UI modules therefore compile against D3D9 names even when they only need higher-level renderer behavior.

The concrete `DX9RENDER` implementation owns:

- the `IDirect3D9` object and `IDirect3DDevice9`
- device creation/reset/release
- presentation parameters
- texture, cube texture, volume texture, surface, vertex buffer, and index buffer lifetime
- fixed-function transforms, lights, materials, render states, sampler states, and texture stage states
- render target stack and post-process render targets
- font rendering and progress/loading UI
- shader/effect technique execution

### Current non-Windows renderer path

Non-Windows builds still use a native D3D9 API through compatibility layers, not an API-neutral renderer.

`cmake/linux.cmake` supports:

- Gallium Nine from Mesa when `STORM_MESA_NINE=ON`
- DXVK Native otherwise

Both paths provide D3D9-compatible headers/libraries to the existing DX9 renderer code.

## Rendering architecture: migration implications

A future OpenGL renderer should not start as a direct replacement of `s_device.cpp`. The public renderer interface must first stop leaking D3D9-specific types into the rest of the engine.

See `docs/plans/renderer-backend-abstraction-plan.md` for the staged implementation plan, neutral type list, compatibility policy, and suggested first PRs.

Recommended migration direction:

1. Introduce backend-neutral renderer types and handles.
   - primitive type
   - texture format
   - index format
   - buffer usage
   - render target handle
   - shader/program handle
   - viewport and clear flags
   - matrix/vector values based on engine math types
2. Add neutral overloads or a new neutral renderer interface while keeping the DX9 implementation working.
3. Convert call sites from D3D9 constants/types to neutral renderer types in small groups.
4. Split the renderer into:
   - frontend/service/resource-handle layer
   - backend interface
   - `D3D9Backend`
   - future `OpenGLBackend`
5. Migrate ImGui/editor/config-app rendering to backend-specific adapters.
6. Migrate technique/effect handling and shaders to a backend-neutral representation or GLSL-compatible path.

The current `VDX9RENDER` name and service name (`"dx9render"`) are part of the compatibility surface. Renaming them should be a later step after a neutral service interface exists.

## Editor and config UI

The in-engine editor is in `src/libs/editor` and uses ImGui. On Windows it currently includes and calls the DX9 ImGui backend.

The config app in `src/apps/configapp` is a separate SDL + ImGui application. It creates its own D3D9 device and is therefore a separate migration surface from the main game renderer.

For an OpenGL migration, the config app and editor are good early targets because they can move to SDL OpenGL + `imgui_impl_opengl3` without touching the full game rendering path.

## Scripting and script-visible APIs

The core and modules expose C++ functions to the script system through `SCRIPT_LIBRIARY` implementations and `core.SetScriptFunction(...)`.

Example renderer script functions are registered in `DX9RENDER_SCRIPT_LIBRIARY::Init()` in `src/libs/renderer/src/s_device.cpp`, including functions such as texture lookup/release and renderer print helpers.

The legacy scripting references are in:

- `docs/syntax.txt`
- `docs/core.txt`
- `docs/subscribe.txt`

## Data/assets at runtime

The build and Conan generation process copy several source-controlled assets into the runtime output directory:

- render techniques from `src/techniques`
- shared resource headers from `src/libs/shared_headers/include/shared`
- ImGui backend sources into the CMake build tree
- selected third-party runtime binaries

Game assets from supported games are still required separately to run `engine.exe`.

## Contributor guidelines for architecture changes

When changing architecture-level code:

- Prefer adding narrow compatibility seams before large rewrites.
- Keep old behavior compiling while introducing a new abstraction.
- Do not expose backend-specific API types from generic headers.
- Keep `core`, entity, service, and module registration behavior intact unless the change explicitly targets them.
- Update both this document and `docs/project-structure.md` when files, modules, or ownership boundaries move.
