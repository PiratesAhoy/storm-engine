# Renderer backend abstraction plan

This document describes a staged plan for abstracting the current DirectX 9 renderer so the engine can later support other rendering backends such as OpenGL or WebGPU.

The goal is not to replace the renderer in one large rewrite. The goal is to first create an API-neutral renderer boundary while keeping the current D3D9 renderer working, then move implementation details behind backend interfaces, and only then add new backends.

## Quick progress checklist

Legend: `[x]` complete, `[~]` in progress, `[ ]` not started.

- [x] Phase 0 - baseline and safety checks
  - [x] Captured renderer coupling counts and D3D leakage metrics.
  - [x] Documented build/smoke-test baseline and current platform blockers.
  - [x] Linked this plan from the main architecture/project documentation.
- [x] Phase 1 - neutral renderer vocabulary
  - [x] Added D3D-free `renderer/render_handles.hpp`.
  - [x] Added D3D-free `renderer/render_types.hpp`.
  - [x] Added renderer handle tests for type safety, invalid/default semantics, and slot `0` validity.
  - [x] Verified the neutral headers do not pull in D3D headers.
  - [x] Verified the Windows build still works. Note: Windows verification currently requires the human user because this agent environment is macOS-only.
- [x] Phase 2 - neutral resource/draw APIs beside legacy APIs
  - [x] Added typed vertex/index buffer overloads beside legacy `int32_t` APIs.
  - [x] Added legacy signed-ID bridge helpers (`-1` invalid, slot `0` valid).
  - [x] Migrated `IVBufferManager` to typed vertex/index buffer handles.
  - [x] Fixed the `IVBufferManager` slot-0 release hazard by using `.IsValid()` instead of truthiness.
  - [x] Add typed `TextureHandle` wrappers around the existing texture table.
  - [x] Migrate one contained texture owner to typed texture handles.
  - [x] Add neutral clear/viewport/draw-UP overloads after resource-handle seams are stable.
- [ ] Phase 3 - convert low-risk call sites to neutral APIs
  - [ ] Move simple debug/UI/helper paths away from D3D constants and raw resource IDs.
  - [ ] Track remaining D3D usage outside `src/libs/renderer` after each slice.
- [ ] Phase 4 - isolate raw D3D access behind compatibility wrappers
  - [ ] Audit non-renderer callers of raw D3D pointer APIs.
  - [ ] Move raw D3D access into compatibility-only interfaces or renderer-internal headers.
- [ ] Phase 5 - split `DX9RENDER` into frontend and D3D9 backend
  - [ ] Introduce a backend-neutral interface for already-neutralized paths.
  - [ ] Move D3D9 resource/device ownership behind a D3D9 backend implementation.
- [ ] Phase 6 - abstract shader and technique execution
  - [ ] Define the shader/effect migration strategy before adding non-D3D backends.
- [ ] Phase 7 - backend selection and first non-D3D backend
  - [ ] Add explicit backend selection once the D3D9 seam is proven.
  - [ ] Start OpenGL/WebGPU only after D3D9 works behind the neutral frontend.

Current blocker: macOS can configure and run focused tests, but full renderer targets still hit legacy `d3d9.h` includes in `dx9render.h` and `platform/d3dx9.hpp`. Windows remains the full-renderer verification path for now, and that verification can only be performed by the human user until a Windows CI/agent environment is available.

## Current state

The current renderer is centered on the `VDX9RENDER` service interface and `DX9RENDER` implementation.

Important files:

- `src/libs/renderer/include/dx9render.h` - public renderer service interface.
- `src/libs/renderer/src/s_device.h` - concrete renderer state and D3D9-owned objects.
- `src/libs/renderer/src/s_device.cpp` - D3D9 device lifecycle, resources, draw calls, render state, post-processing, script functions, and presentation.
- `src/libs/renderer/src/effects.*` - Windows D3DX effect path.
- `src/libs/renderer/src/technique.*` - non-Windows custom technique parser for the D3D9-compatible path.
- `src/techniques/` - effect/technique assets copied into the runtime resource tree.
- `src/libs/editor/` and `src/apps/configapp/` - ImGui DX9 backend users that must also be abstracted or migrated.

Known coupling points from codebase inspection:

- `VDX9RENDER` is defined in `src/libs/renderer/include/dx9render.h` and is requested across the engine with `core.GetServiceX<VDX9RENDER>()`.
- The public renderer header exposes D3D9 concepts directly: `D3DMATRIX`, `D3DLIGHT9`, `D3DMATERIAL9`, `D3DPRIMITIVETYPE`, `D3DFORMAT`, `D3DPOOL`, `D3DVIEWPORT9`, `IDirect3DTexture9`, `IDirect3DSurface9`, vertex/pixel shader interfaces, and other D3D types.
- `src/libs/renderer/include/dx9render.h` has a large public `D3D SECTION` that gives callers direct access to D3D-like device operations.
- `DX9RENDER::InitDevice` creates the D3D9 object/device in `src/libs/renderer/src/s_device.cpp` and owns presentation, render target, texture, buffer, shader, and state lifetime.
- Renderer resources are still mostly exposed as plain `int32_t` table indexes. Textures, vertex buffers, and index buffers are stored in fixed arrays in `src/libs/renderer/src/s_device.h`, and legacy APIs use `-1` as an invalid value in some places while slot `0` is a valid resource. The first buffer seam now has typed `VertexBufferHandle` and `IndexBufferHandle` wrappers beside the legacy APIs, with `IVBufferManager` migrated to the typed path.
- macOS no longer tries to build native D3D9 compatibility layers. `cmake/linux.cmake` treats Gallium Nine and DXVK Native as Linux-only and creates a no-op `dependencies` target on macOS. Non-Windows renderer builds still require further D3D header isolation before a real macOS renderer target can build.

## Current implementation status - 2026-06-13

Completed foundation:

- `src/libs/util/include/handle.hpp` provides the strongly typed integer-compatible `storm::Handle<Tag, HandleType = uint32_t>` template.
- `src/libs/renderer/include/renderer/render_handles.hpp` defines neutral renderer resource handles for textures, vertex buffers, index buffers, unified buffers, render targets, shaders, and programs.
- `src/libs/renderer/include/renderer/render_types.hpp` defines the initial D3D-free renderer vocabulary for backend type, primitive/index/texture/buffer concepts, transforms, clear flags, rectangles, viewports, and colors.
- `src/libs/renderer/testsuite/render_handles.cpp` verifies handle layout/type-safety, default-invalid semantics, slot `0` validity, independent handle domains, and legacy signed-ID bridging.
- `src/libs/renderer/include/dx9render.h`, `src/libs/renderer/src/s_device.h`, and `src/libs/renderer/src/s_device.cpp` now expose typed vertex/index buffer overloads beside the legacy `int32_t` methods.
- `src/libs/renderer/src/iv_buffer_manager.cpp` creates, locks, unlocks, draws, and releases vertex/index buffers through typed handles. Its destructor now uses `.IsValid()` instead of truthiness, fixing the slot-0 release hazard.

Current verification:

- Direct renderer handle syntax check passed on macOS.
- Direct renderer handle test binary passed on macOS: `All tests passed (19 assertions in 3 test cases)`.
- `cmake -S . -B build-blaze -DCMAKE_BUILD_TYPE=Debug` passed on macOS.
- `cmake --build build-blaze --target util-test && build-blaze/Debug/util-test` passed on macOS: `All tests passed (74 assertions in 6 test cases)`.
- `cmake --build build-blaze --target dependencies` is a no-op/pass on macOS after disabling native D3D9 compatibility layers there.
- Full `renderer-test` on macOS remains blocked by legacy D3D headers in `src/libs/renderer/include/dx9render.h` and `src/libs/util/include/platform/d3dx9.hpp` (`fatal error: 'd3d9.h' file not found`).
- The Windows build has been verified by the user after the typed buffer-handle slice. This is currently a human-only verification step because the available agent environment is macOS-only.

Next recommended slice: continue migrating small local texture owners to typed `TextureHandle` overloads before broad draw/state migration.

## Goals

1. Preserve current D3D9 behavior during the migration.
2. Create a renderer API that does not expose D3D9/OpenGL/WebGPU types to game systems.
3. Split high-level renderer responsibilities from backend-specific device/resource implementation.
4. Allow multiple backends to coexist during migration.
5. Make backend selection explicit in build/runtime configuration.
6. Keep risky systems (sea, weather, post-process, shadows, techniques) behind stable compatibility seams until they can be migrated safely.

## Non-goals for the first stages

- Do not rewrite every renderer caller immediately.
- Do not remove `VDX9RENDER` or the `"dx9render"` service name in the first PR.
- Do not implement OpenGL/WebGPU before the neutral renderer boundary exists.
- Do not convert all shader/effect files in the first pass.
- Do not change gameplay behavior or asset formats as part of the abstraction seam.

## Target architecture

Long term, rendering should be split into three layers.

### 1. Renderer service / frontend

Owns engine-facing behavior:

- service registration and script-visible functions
- texture/font/model-facing renderer handles
- high-level draw helpers used by gameplay/UI modules
- render target stack from the engine point of view
- loading/progress UI
- post-process orchestration
- frame begin/end integration with core/entities
- compatibility wrappers for legacy calls during migration

This layer should expose engine-owned types only.

### 2. Backend-neutral renderer interface

Defines the minimal operations required by the frontend:

- device/context creation and destruction
- frame begin/end/present
- resource creation/destruction
- texture upload/update/readback
- buffer creation/update/map/unmap
- render target creation/binding
- pipeline/shader binding
- draw and indexed draw
- state application
- viewport/scissor/clear
- debug labels and diagnostics where available

This layer should not mention `IDirect3D*`, OpenGL object names, WebGPU handles, or platform-specific API structs in public headers used by game modules.

### 3. Backend implementations

Concrete backends implement the neutral backend interface:

- `D3D9Backend` - wraps existing D3D9 behavior first.
- `OpenGLBackend` - future SDL OpenGL context and GL resource implementation.
- `WebGPUBackend` - future WebGPU device/surface/pipeline implementation.

Backend-specific headers should stay under backend implementation directories and should not be included by general gameplay modules.

A possible future directory shape:

```text
src/libs/renderer/include/render/
  render_service.h
  render_types.hpp
  render_handles.hpp
  render_backend.h
  render_pipeline.h

src/libs/renderer/src/frontend/
  render_service.cpp
  legacy_dx9_compat.cpp
  texture_manager.cpp
  font_renderer.cpp
  technique_manager.cpp

src/libs/renderer/src/backends/d3d9/
  d3d9_backend.h
  d3d9_backend.cpp
  d3d9_type_conversions.h

src/libs/renderer/src/backends/opengl/
  opengl_backend.h
  opengl_backend.cpp

src/libs/renderer/src/backends/webgpu/
  webgpu_backend.h
  webgpu_backend.cpp
```

This exact shape can change, but the ownership boundary should remain: engine-facing headers are neutral; backend directories own API-specific types.

## Neutral types to introduce first

Start with small types that can map directly to D3D9 without behavior changes.

Suggested initial header: `src/libs/renderer/include/renderer/render_types.hpp`.

Initial enums/value types:

- `RenderBackendType`
  - `D3D9`
  - `OpenGL`
  - `WebGPU`
- `PrimitiveType`
  - `TriangleList`
  - `TriangleStrip`
  - `LineList`
  - `LineStrip`
  - `PointList`
- `IndexFormat`
  - `UInt16`
  - `UInt32`
- `TextureFormat`
  - start with formats currently used by `TextureCreate`, render targets, and texture files
- `TextureUsage`
  - sampled
  - render target
  - depth/stencil
  - dynamic/updateable
- `BufferUsage`
  - static
  - dynamic
  - stream
- `BufferType`
  - vertex
  - index
- `TransformType`
  - world
  - view
  - projection
- `ClearFlags`
  - color
  - depth
  - stencil
- `Viewport`
- `Rect`
- `Color`
- `SamplerState`
- `RenderState`
- `BlendMode`, `CullMode`, `DepthState` as later refinements

Initial handle types:

- `TextureHandle`
- `VertexBufferHandle`
- `IndexBufferHandle`
- `BufferHandle` if vertex/index buffers are unified later
- `RenderTargetHandle`
- `ShaderHandle` / `ProgramHandle` later
- `FontHandle` can remain higher-level if font rendering stays in the frontend

Handles should use the existing strongly typed integer-compatible handle template from `src/libs/util/include/handle.hpp` rather than raw integers or ad-hoc typedefs. Suggested aliases in `src/libs/renderer/include/renderer/render_handles.hpp`:

```cpp
namespace storm::render
{
using TextureHandle = storm::Handle<struct TextureHandleTag>;
using VertexBufferHandle = storm::Handle<struct VertexBufferHandleTag>;
using IndexBufferHandle = storm::Handle<struct IndexBufferHandleTag>;
using RenderTargetHandle = storm::Handle<struct RenderTargetHandleTag>;
using ShaderHandle = storm::Handle<struct ShaderHandleTag>;
using ProgramHandle = storm::Handle<struct ProgramHandleTag>;
} // namespace storm::render
```

The handle value should remain an engine/frontend resource token. It must not expose `IDirect3D*` pointers, OpenGL object names, WebGPU handles, or any backend-native object. Backend-native objects stay in renderer-owned resource tables or backend implementation classes.

## Resource handle and tracking strategy

Typed handles should be the public resource identity for the neutral renderer layer.

Use them to replace or wrap these existing integer IDs first:

- texture IDs returned by `TextureCreate` and consumed by `TextureSet`, `TextureRelease`, `TextureIncReference`, `GetBaseTexture`, `ImageBlt`, and related compatibility paths
- vertex buffer IDs returned by `CreateVertexBuffer` and consumed by lock/unlock/draw/release calls
- index buffer IDs returned by `CreateIndexBuffer` and consumed by lock/unlock/draw/release calls
- render target handles once render targets move from raw surfaces to frontend-owned resources

The immediate migration model can keep the current D3D9 tables. A typed handle's `Value()` can map to the existing slot index while the neutral API grows around it. That gives type safety without forcing a large resource-manager rewrite in the same patch.

Important validity rule: slot `0` is valid. Do not use truthiness to test resource IDs. The handle template uses `std::numeric_limits<uint32_t>::max()` as the invalid sentinel, so callers should use `handle.IsValid()` and reset handles with `handle.Invalidate()` or assignment to `Handle::Invalid()`.

This avoids current legacy pitfalls such as checking `if (vertex_buffer_)` before releasing a buffer. A buffer allocated in slot `0` would be skipped by that pattern. The typed form should be explicit:

```cpp
if (vertexBuffer_.IsValid())
{
    renderer.ReleaseVertexBuffer(vertexBuffer_);
    vertexBuffer_.Invalidate();
}
```

Pair typed handles with resource-table metadata so the frontend can track lifetime and diagnostics independently of the backend API:

- backend resource pointer/object, private to the backend or compatibility layer
- resource kind and creation parameters needed for diagnostics or device restore
- reference count or ownership policy
- debug name/source path
- estimated memory size
- loaded/resident state
- lock/map state where applicable

The current `STEXTURE` table already contains part of this model (`d3dtex`, `name`, `hash`, `ref`, `dwSize`, `isCubeMap`, `loaded`). The first pass should preserve that data and put a typed handle boundary around it. Later, if stale handle detection becomes necessary, add a generation counter or a wider encoded handle value instead of exposing backend pointers.

Do not use typed renderer handles as backend-native object handles. For example:

- D3D9 still owns `IDirect3DBaseTexture9 *` and `IDirect3DVertexBuffer9 *` internally.
- OpenGL may own `GLuint` names internally.
- WebGPU may own `WGPUTexture`, `WGPUTextureView`, buffers, bind groups, and pipelines internally.

Only the renderer frontend and neutral interfaces should traffic in `TextureHandle`, `VertexBufferHandle`, `IndexBufferHandle`, and related engine-level handles.

## Compatibility policy

For migration safety, use an additive compatibility policy.

1. Keep `VDX9RENDER` available while adding neutral APIs.
2. Add neutral overloads next to legacy D3D9 signatures.
3. Implement neutral overloads by translating to the existing D3D9 implementation.
4. Migrate call sites one subsystem at a time.
5. After call sites stop using a legacy D3D9 API, move that API into a compatibility-only header or remove it.
6. Rename `VDX9RENDER`/service names only after most consumers use the neutral renderer interface.

Do not make a change that requires all 500+ renderer service references or thousands of D3D constants to be fixed in one patch.

## Staged implementation plan

### Phase 0: Baseline and safety checks

Purpose: make sure the starting point is measurable and reproducible.

Tasks:

- Record the current renderer coupling counts in this document or an issue.
- Identify build commands for Windows and Linux.
- Identify a minimal runtime smoke test scene or launch path.
- Add/confirm documentation links from `README.md`, `docs/architecture.md`, and `docs/project-structure.md`.

Validation:

- Existing build still passes.
- Existing renderer still initializes with D3D9.
- No code changes beyond docs/diagnostics in this phase.

#### Phase 0 baseline snapshot - 2026-06-12

Scope: repository root on branch `feature/rendering`, source searches under `src/` unless otherwise noted. Local machine: macOS on Apple Silicon, CMake 3.31.4, Debug `build-blaze` tree.

Working tree at capture time:

- `git status --short` showed only local `.DS_Store` noise.
- No source-code changes are part of Phase 0.

Coupling counts:

| Metric | Count | Command |
| --- | ---: | --- |
| `VDX9RENDER` occurrences in `src/` | 528 | `rg -o '\bVDX9RENDER\b' src \| wc -l` |
| Files with `VDX9RENDER` in `src/` | 370 | `rg -l '\bVDX9RENDER\b' src \| wc -l` |
| Concrete `DX9RENDER` occurrences in `src/` | 206 | `rg -o '\bDX9RENDER\b' src \| wc -l` |
| Files with concrete `DX9RENDER` in `src/` | 2 | `rg -l '\bDX9RENDER\b' src \| wc -l` |
| `IDirect3D` occurrences in `src/` | 338 | `rg -o 'IDirect3D' src \| wc -l` |
| Files with `IDirect3D` in `src/` | 66 | `rg -l 'IDirect3D' src \| wc -l` |
| `GetD3DDevice` occurrences in `src/` | 3 | `rg -o 'GetD3DDevice' src \| wc -l` |
| Raw `d3d9->` calls in `src/libs/renderer/` | 119 | `rg -o 'd3d9->' src/libs/renderer \| wc -l` |
| D3D-style tokens in `src/` | 2654 | `rg -o '\b(D3D[A-Z0-9_]*\|D3DFMT_[A-Z0-9_]+\|D3DPT_[A-Z0-9_]+\|D3DTS_[A-Z0-9_]+\|D3DPOOL_[A-Z0-9_]+\|D3DUSAGE_[A-Z0-9_]+\|D3DCLEAR_[A-Z0-9_]+\|D3DFVF_[A-Z0-9_]+)\b' src \| wc -l` |
| D3D-style tokens outside `src/libs/renderer/` | 1613 | same D3D-token search piped through `rg -v '^src/libs/renderer/' \| wc -l` |
| Files with D3D-style tokens outside `src/libs/renderer/` | 173 | `rg -l '<same D3D-token pattern>' src \| rg -v '^src/libs/renderer/' \| wc -l` |
| Public renderer resource APIs still exposing raw IDs/pointers | 13 | `rg -n 'virtual (int32_t\|bool\|void\|IDirect3D[^ ]+ \*) (TextureCreate\|TextureSet\|TextureRelease\|TextureIncReference\|CreateVertexBuffer\|CreateIndexBuffer\|GetVertexBuffer\|GetVertexBufferFVF\|LockVertexBuffer\|UnLockVertexBuffer\|GetVertexBufferSize\|LockIndexBuffer\|UnLockIndexBuffer\|ReleaseVertexBuffer\|ReleaseIndexBuffer\|GetBaseTexture\|GetTextureFromID\|ImageBlt)\(' src/libs/renderer/include/dx9render.h \| wc -l` |
| Non-renderer direct D3D include sites | 6 | `rg -n 'd3d9\|d3dx9\|d3d9types\|d3dx9tex' src \| rg '#\s*include' \| rg -v '^src/libs/renderer/' \| wc -l` |

Known coupling anchors:

- Public renderer service: `src/libs/renderer/include/dx9render.h` defines `VDX9RENDER` and exposes both high-level renderer calls and raw D3D-like device operations.
- Concrete renderer implementation: `src/libs/renderer/src/s_device.h` and `src/libs/renderer/src/s_device.cpp` own the current D3D9 device, texture table, buffer tables, render targets, post-process resources, and compatibility operations.
- Non-Windows D3D compatibility setup: `CMakeLists.txt` exposes `STORM_MESA_NINE`. `cmake/linux.cmake` selects Gallium Nine or DXVK Native on Linux while still presenting a D3D9-shaped API to the renderer; on macOS those native D3D9 compatibility layers are intentionally disabled.

Build and verification commands:

| Purpose | Command | Phase 0 result |
| --- | --- | --- |
| Configure local non-Windows tree | `cmake -S . -B build-blaze` | Passed on macOS. Conan dependencies resolved from cache; macOS skips native D3D9 compatibility layers. |
| Build/run focused unit tests | `cmake --build build-blaze --target util-test -- -j2` | Passed: `All tests passed (74 assertions in 6 test cases)`. |
| Build DXVK Native dependency target | `cmake --build build-blaze --target dependencies -- -j2` | Passed/no-op on macOS after native D3D9 compatibility layers were disabled there. Linux still uses the Gallium Nine/DXVK Native paths. |
| Build renderer target locally | `cmake --build build-blaze --target renderer -- -j2` | Still blocked on macOS by direct legacy D3D headers: `fatal error: 'd3d9.h' file not found` from `dx9render.h` and `platform/d3dx9.hpp`. |
| Windows build path | Open the repo root as a CMake project in Visual Studio 2019 and select `engine.exe` as startup item. | Verified by the user after Phase 1 scaffolding and after the first Phase 2 typed buffer-handle slice. This is human-only until Windows CI/agent access exists. |

Runtime smoke-test baseline:

- Minimal Windows smoke path is currently the documented `engine.exe` launch from Visual Studio with DirectX 9 runtime libraries installed.
- Runtime launch also requires assets from one of the supported games, so Phase 0 cannot define a repo-only renderer smoke test yet.
- The first practical smoke target should be documented before Phase 2 caller migrations: launch `engine.exe` with a known supported game resource tree, confirm `DX9RENDER::InitDevice` succeeds, render at least one frame, and close cleanly.
- On macOS, a comparable renderer smoke test requires isolating the remaining legacy D3D headers or adding a real non-D3D backend first. Linux still depends on a working Gallium Nine/DXVK Native setup for the current renderer path.

Documentation link check:

- `README.md` links this renderer backend abstraction plan.
- `docs/architecture.md` links this renderer backend abstraction plan.
- `docs/project-structure.md` links this renderer backend abstraction plan.
- `docs/dependencies.md` also links this renderer backend abstraction plan for dependency/migration context.

Phase 0 status:

- Baseline counts are recorded and reproducible.
- Local configure and a focused test target are verified.
- Full local renderer build and runtime smoke are not green on this macOS machine because legacy public headers still include D3D9 headers directly.
- Phase 1 is complete and Phase 2 has started. Windows build verification is the authoritative full-renderer check until the remaining macOS D3D header blockers are isolated, but this currently requires the human user to run it.

### Phase 1: Add neutral renderer vocabulary

Status: complete for the initial scaffold.

Purpose: create shared language without changing behavior.

Tasks:

- Add neutral renderer type headers under `src/libs/renderer/include/renderer/`.
- Add `src/libs/renderer/include/renderer/render_handles.hpp` with typed aliases over `storm::Handle<Tag>`.
- Add D3D9 conversion helpers under `src/libs/renderer/src/backends/d3d9/` or a temporary internal file.
- Add unit tests for pure conversion functions where practical.
- Add compile-time checks that renderer handle domains cannot be mixed and that default handles are invalid while `0` remains a valid value.
- Keep all D3D9 includes out of neutral public headers.

Suggested first conversions:

- `PrimitiveType` -> `D3DPRIMITIVETYPE`
- `IndexFormat` -> `D3DFORMAT`
- `TransformType` -> `D3DTRANSFORMSTATETYPE`
- `ClearFlags` -> `D3DCLEAR_*`
- `TextureFormat` -> `D3DFORMAT` for the subset currently used

Validation:

- Include neutral headers from a non-renderer module without pulling in `<d3d9.h>`: verified with a D3D-free header probe on macOS.
- Existing D3D9 renderer build still passes: verified by the user on Windows.

Implemented:

- `src/libs/renderer/include/renderer/render_handles.hpp`
- `src/libs/renderer/include/renderer/render_types.hpp`
- `src/libs/renderer/testsuite/render_handles.cpp`

### Phase 2: Add neutral draw/resource APIs beside legacy APIs

Status: complete for the initial neutral resource/draw API slice.

Purpose: prove neutral types can drive the current renderer.

Tasks:

- Add neutral overloads to the renderer service/frontend for the smallest useful draw/resource paths.
- Implement each neutral overload by mapping to the current D3D9 implementation.
- Use typed handles in the neutral resource overloads; convert to legacy `int32_t` only inside compatibility wrappers or the current D3D9 implementation.
- Keep legacy methods unchanged.

Good first APIs:

- clear flags
- viewport get/set
- `DrawPrimitiveUP`
- `DrawIndexedPrimitiveUP`
- texture set by existing texture id
- simple vertex/index buffer create/lock/unlock wrappers
- texture, vertex buffer, and index buffer release wrappers that call `IsValid()` instead of relying on `-1` or truthiness

Avoid first:

- post-processing
- effects/techniques
- cube/volume textures
- raw surface operations
- shader creation
- fixed-function lighting

Validation:

- Build passes.
- Existing code path still uses legacy methods unless explicitly migrated.
- At least one small caller can use the neutral overload without behavior change.

Completed first slice:

- Added typed `VertexBufferHandle` and `IndexBufferHandle` overloads for create, lock, unlock, release, and draw-buffer calls while preserving the legacy `int32_t` API.
- Added `HandleFromLegacyId` and `HandleToLegacyId` bridging helpers. Legacy `-1` maps to typed invalid handles, and legacy slot `0` remains a valid typed handle.
- Migrated `IVBufferManager` from `renderer_handle` fields to typed vertex/index buffer handles.
- Replaced `IVBufferManager` truthiness-based release checks with `.IsValid()`.
- Added typed `TextureHandle` overloads around the existing texture table for create, set, release, reference increment, image blit, and base-texture lookup while preserving legacy `int32_t` methods.
- Migrated `SEAFOAM::carcassTexture` to `storm::render::TextureHandle`, using `TextureCreateHandle`, typed `TextureSet` / `TextureRelease`, and `.IsValid()` for release.

Remaining recommended Phase 2 work:

- The initial Phase 2 resource/draw API slice is complete. Broader texture-array and multi-texture migrations remain deferred to subsystem-specific passes.
- Begin Phase 3 with low-risk debug/UI/helper caller migrations.

Completed neutral draw/state slice:

- Added D3D9-backed neutral `ClearFlags`/`Color` clear overloads.
- Added neutral `Viewport` get/set overloads.
- Added neutral `PrimitiveType` and `IndexFormat` draw-UP overloads.
- Migrated the main frame clear path to the neutral clear overload while preserving the legacy APIs.

Contained one-texture owners that are good candidates for the next typed `TextureHandle` slices:

Migration pattern for each owner:

- Read the owning header and implementation first; confirm the texture is a local owner field and not shared externally.
- Change only that one field from `int32_t` / `uint32_t` to `storm::render::TextureHandle`.
- Use default invalid construction where possible. If the constructor previously relied on `0`/`-1` to keep `Release()` safe before `Initialize()`, also initialize nullable service pointers such as `renderer` to `nullptr`.
- Replace `TextureCreate(...)` with `TextureCreateHandle(...)`.
- Leave existing `TextureSet(stage, texture)` calls structurally unchanged; overload resolution should select the typed path.
- Prefer release through the typed overload without a handle validity guard because `TextureRelease(TextureHandle)` is an invalid-handle no-op. Keep guards for nullable renderer/service pointers.
- After release, reset the handle with `handle.Invalidate()` or `handle = {}` if the owner can be released more than once or reinitialized.
- Do not migrate neighbouring particle systems, texture arrays, or unrelated resource IDs in the same slice.
- Mark exactly one checklist item complete, run lightweight verification, then stop for manual review before continuing.

- [x] `src/libs/sink_effect/src/t_sink.{h,cpp}` - `TSink::texture`
- [x] `src/libs/worldmap/src/wdm_warring_ship.{h,cpp}` - `WdmWarringShip::texture`
- [x] `src/libs/worldmap/src/wdm_wind_rose.{h,cpp}` - `WdmWindRose::shadowTexture`
- [x] `src/libs/worldmap/src/wdm_icon.{h,cpp}` - `WdmIcon::texture`
- [x] `src/libs/animals/src/t_butterflies.{h,cpp}` - `TButterflies::texture`
- [x] `src/libs/water_rings/src/water_rings.{h,cpp}` - `WaterRings::ringTexture`
- [x] `src/libs/location/src/blood.{h,cpp}` - `Blood::texID`
- [x] `src/libs/blot/src/blots.{h,cpp}` - `Blots::textureID`
- [x] `src/libs/sea_ai/src/ai_ship_camera_controller.{h,cpp}` - `AIShipCameraController::iCrosshairTex`
- [x] `src/libs/worldmap/src/wdm_storm.{h,cpp}` - `WdmStorm::rainTexture`
- [x] `src/libs/worldmap/src/wdm_ship.{h,cpp}` - `WdmShip::wmtexture`
- [x] `src/libs/rigging/src/rope.{h,cpp}` - `ROPE::texl`
- [x] `src/libs/battle_interface/src/image/material.{h,cpp}` - `BIImageMaterial::m_nTextureID`
- [x] `src/libs/weather/src/water_flare.{h,cpp}` - `WATERFLARE::iFlareTex`
- [x] `src/libs/sea_creatures/src/sharks.{h,cpp}` - `SHARKS::trackTx`
- [x] `src/libs/sea_ai/src/ai_balls.{h,cpp}` - `AI_BALLS::dwTextureIndex`
- [x] `src/libs/rigging/src/flag.{h,cpp}` - `FLAG::texl`
- [x] `src/libs/rigging/src/vant.{h,cpp}` - `VANT::texl`
- [x] `src/libs/location/src/grass.{h,cpp}` - `GRASS::texture`; texture-stage clears remain legacy `-1` by API design.
- [x] `src/libs/dialog/src/legacy_dialog.{h,cpp}` - `LegacyDialog::interfaceTexture_`
- [x] `src/libs/renderer/src/font.{h,cpp}` - `FONT::textureHandle_`

Do not fold texture arrays or multi-texture systems into these slices. Deferred areas include `seafoam_ps`, `seps`, weather sun/rain/sky, worldmap sea/wind UI, rigging sail, and xinterface nodes; migrate those as separate subsystem-specific passes.

### Phase 3: Convert low-risk call sites to neutral APIs

Purpose: reduce D3D9 leakage outside the renderer without changing the backend.

Start with simple rendering helpers and debug/UI paths, not major scene systems.

Candidate areas:

- debug lines/vectors
- simple sprite/rect drawing
- small UI helpers in `battle_interface` or `xinterface`
- isolated helper functions that currently pass `D3DPT_*`, `D3DFVF_*`, or clear flags
- isolated renderer helpers that store texture or buffer IDs as `int32_t`, such as font/BMFont texture handles. `IVBufferManager` vertex/index buffer IDs have already moved to typed handles.

Tasks:

- Replace D3D constants with neutral enums in selected call sites.
- Replace raw D3D structs with neutral structs where the mapping is clear.
- Replace selected raw resource IDs with typed renderer handles, starting with places that already own/release resources locally.
- Keep changes small enough to review subsystem by subsystem.

Validation:

- Build after each subsystem conversion.
- Run any available UI/render smoke test.
- Track remaining D3D usages outside `src/libs/renderer`.

Progress:

- [x] `src/libs/lighter/src/l_geometry.cpp` - migrate `LGeometry::DrawNormals` to the neutral `LineList` primitive type; retain the legacy FVF bitmask until vertex-layout vocabulary is introduced.
- [x] `src/libs/location/src/wide_screen.cpp` - migrate viewport access and the `TriangleList` primitive type; retain the legacy FVF bitmask until vertex-layout vocabulary is introduced.
- [x] `src/libs/location/src/ptc_data.cpp` - migrate debug triangle/edge primitive types; retain the legacy FVF and world-transform APIs for a later slice.

### Phase 4: Isolate raw D3D access behind compatibility wrappers

Purpose: stop non-renderer modules from owning D3D objects directly.

Current public APIs that need isolation include:

- `GetD3DDevice()`
- `GetVertexBuffer()` returning `IDirect3DVertexBuffer9 *`
- texture/surface methods returning or accepting `IDirect3DTexture9`, `IDirect3DSurface9`, `IDirect3DBaseTexture9`, `IDirect3DCubeTexture9`, `IDirect3DVolumeTexture9`
- shader methods returning or accepting D3D shader interfaces
- `GetEffectPointer()` returning `ID3DXEffect *`

Tasks:

- Audit every non-renderer caller of raw D3D pointer APIs.
- Audit every non-renderer caller that persists renderer resource IDs as `int32_t`.
- For each caller, decide whether it needs:
  - a new high-level renderer operation,
  - an opaque handle lookup,
  - a readback/upload helper,
  - or temporary backend-specific compatibility.
- Move raw D3D methods behind a clearly named compatibility interface, for example `VD3D9RenderCompatibility` or an internal renderer-only header.

Validation:

- Non-renderer modules no longer include D3D headers for the migrated paths.
- Remaining D3D includes outside renderer are documented and intentionally deferred.

### Phase 5: Split `DX9RENDER` into frontend and D3D9 backend

Purpose: create the real backend seam while preserving behavior.

Tasks:

- Introduce `IRenderBackend` with only operations required by the already-neutral frontend paths.
- Move D3D9 device/context/resource code out of the service/frontend into `D3D9Backend` incrementally.
- Keep texture/font/resource handle management and resource tracking in the frontend unless there is a strong reason to move it.
- Let backend implementations own backend-native objects behind frontend-owned typed handles.
- Let the frontend call backend methods for device creation, resource creation, state application, draw, and present.

Suggested first backend methods:

- `initialize(window, width, height, options)`
- `shutdown()`
- `beginFrame()` / `endFrame()` / `present()`
- `clear(...)`
- `setViewport(...)`
- `createTexture(...)`
- `destroyTexture(...)`
- `setTexture(stage, handle)`
- `createBuffer(...)`
- `destroyBuffer(...)`
- `mapBuffer(handle, ...)` / `unmapBuffer(handle)`
- `updateBuffer(...)`
- `draw(...)`
- `drawIndexed(...)`

Validation:

- D3D9 backend produces equivalent behavior.
- Frontend owns no raw `IDirect3D9`/`IDirect3DDevice9` fields after the moved subset.
- Backend-native resource objects are not visible in public renderer headers; public APIs use neutral typed handles.
- No new backend is added until the D3D9 backend seam is demonstrably working.

### Phase 6: Abstract shader and technique execution

Purpose: prepare for APIs that do not support D3D9 fixed-function/effects directly.

Tasks:

- Separate technique parsing from backend execution.
- Convert technique passes into neutral pass descriptions:
  - render states
  - sampler states
  - shader/program references
  - constants/uniforms
  - texture bindings
- Keep a D3D9 executor for existing effect/technique assets.
- Add a GLSL/WGSL strategy after the neutral pass model exists.

Possible shader strategies:

1. Manual GLSL/WGSL rewrite for key techniques.
2. HLSL cross-compilation where source syntax permits it.
3. A new backend-neutral material/technique format with generated backend shader sources.

Validation:

- Existing `.fx`/technique behavior remains available on D3D9.
- A small neutral test technique can execute without referencing D3D9 types from the frontend.

### Phase 7: Backend selection and build integration

Purpose: make multiple renderer backends selectable.

Tasks:

- Add a CMake option such as `STORM_RENDER_BACKEND` with values like `D3D9`, `OPENGL`, `WEBGPU`.
- Keep D3D9 as the default until another backend is complete enough to run.
- Move D3D9 dependencies so they are only required for the D3D9 backend.
- Add OpenGL loader/context dependencies only for the OpenGL backend.
- Add WebGPU dependencies only for the WebGPU backend.
- Copy the relevant ImGui backend source (`imgui_impl_dx9`, `imgui_impl_opengl3`, future WebGPU backend) according to selected backend.

Validation:

- D3D9 builds still work with the default option.
- Non-selected backend dependencies are not required.
- CI can build at least the default backend.

### Phase 8: Implement the first non-D3D9 backend

Purpose: validate the abstraction with a real backend.

Recommended first target: OpenGL, because SDL OpenGL context creation and ImGui OpenGL backend are straightforward compared to WebGPU.

OpenGL backend tasks:

- create SDL OpenGL context
- implement frame begin/end/present
- implement clear/viewport/scissor
- implement texture creation/upload/sampling
- implement buffer creation/update/draw
- implement simple shader/program path
- implement render targets with FBOs
- port a minimal sprite/line/debug draw path
- then expand to UI, geometry, post-process, sea/weather/shadows

WebGPU backend tasks later:

- add device/surface/adapter lifecycle
- build explicit pipeline layout model
- use explicit bind groups/resource layouts
- ensure frontend abstractions can describe WebGPU constraints without leaking WebGPU types

Validation:

- Run a minimal scene or render smoke test on the new backend.
- Keep D3D9 backend as a reference until output parity is acceptable.

## Subsystem migration order

Recommended order from lower risk to higher risk:

1. Config app ImGui backend abstraction.
2. In-engine editor ImGui backend abstraction.
3. Debug lines, vectors, rects, sprites.
4. Font and 2D UI rendering.
5. Basic model/geometry draw paths.
6. Render targets and screenshots/readback.
7. Post-processing.
8. Particles.
9. World map.
10. Location/sea/weather/shadow rendering.
11. Technique/effect system and full shader migration.
12. Raw compatibility API removal/renaming.

## D3D9-to-modern-backend semantic issues

Plan for these before claiming backend parity:

- D3D clip depth range and projection matrices vs OpenGL/WebGPU conventions.
- Texture coordinate origin and framebuffer readback orientation.
- Half-texel offsets in old D3D9 screen-space paths.
- Color channel order and packed ARGB/BGRA/RGBA formats.
- Front-face winding and culling defaults.
- Fixed-function lighting/material/texture-stage behavior.
- D3D managed/default pool lifetime vs explicit resource lifetime.
- Device lost/reset handling vs OpenGL/WebGPU context/device loss models.
- Frontend handle lifetime vs backend-native object lifetime. A valid `TextureHandle` should not imply a currently resident backend object during device loss/reset.
- Stale handle reuse. The first pass may use table indexes only, but the resource manager should leave room for generation counters or equivalent validation if reuse bugs appear.
- Shader constants/registers vs uniforms/bind groups.
- Render state mutability vs pipeline object models, especially for WebGPU.
- Lock/unlock buffer and texture APIs vs mapped/staging/update APIs.

## Suggested first implementation PRs

### PR 1: Neutral render type scaffolding

Status: complete for the initial scaffold.

Files likely touched:

- add `src/libs/renderer/include/renderer/render_types.hpp`
- add `src/libs/renderer/include/renderer/render_handles.hpp`
- use `src/libs/util/include/handle.hpp` for all neutral renderer handle aliases
- add internal D3D9 conversion helpers under `src/libs/renderer/src/`
- add small conversion tests if the test structure supports it

Implemented note: the initial D3D-free vocabulary and handle aliases live in `src/libs/renderer/include/renderer/`. D3D9 enum conversion helpers are still deferred; the first implemented bridge is the legacy signed resource-ID bridge in `render_handles.hpp`.

Acceptance criteria:

- Neutral headers do not include `<d3d9.h>`.
- Renderer handles are strongly typed, default-invalid, and do not implicitly convert to raw integers.
- `0` remains a valid handle value; invalid is the handle template's sentinel value.
- D3D9 renderer still builds.
- No gameplay call site migration yet unless needed for compile coverage.

### PR 1b: First typed resource-handle wrappers

Status: complete for vertex/index buffers; texture handles remain the next resource wrapper slice.

Files likely touched:

- `src/libs/renderer/include/dx9render.h` or a new neutral service header
- `src/libs/renderer/src/s_device.h`
- `src/libs/renderer/src/s_device.cpp`
- one small local owner such as `src/libs/renderer/src/iv_buffer_manager.*` or font/BMFont texture ownership code

Acceptance criteria:

- Neutral overloads accept `VertexBufferHandle` and `IndexBufferHandle` while legacy `int32_t` methods remain available. `TextureHandle` overloads are next.
- Compatibility wrappers validate handles with `IsValid()` and convert to raw slot indexes only inside renderer implementation code.
- `IVBufferManager` stops using truthiness checks for migrated vertex/index buffers and releases them only when `.IsValid()`.
- Existing D3D9 behavior remains unchanged; Windows build was verified by the user and cannot currently be run by the macOS-only agent environment.

Implemented files:

- `src/libs/renderer/include/dx9render.h`
- `src/libs/renderer/src/s_device.h`
- `src/libs/renderer/src/s_device.cpp`
- `src/libs/renderer/include/iv_buffer_manager.h`
- `src/libs/renderer/src/iv_buffer_manager.cpp`
- `src/libs/renderer/include/renderer/render_handles.hpp`
- `src/libs/renderer/testsuite/render_handles.cpp`

Validation:

- Direct renderer handle syntax check passed on macOS.
- Direct renderer handle tests passed on macOS: `All tests passed (19 assertions in 3 test cases)`.
- macOS configure passed.
- macOS `util-test` passed: `All tests passed (74 assertions in 6 test cases)`.
- Windows build passed per user verification; this remains a human-only check until Windows CI/agent access exists.
- macOS `renderer-test` remains blocked by legacy D3D header includes, not by this slice.

### PR 2: Neutral clear/viewport/draw-UP path

Files likely touched:

- `src/libs/renderer/include/dx9render.h` or a new neutral service header
- `src/libs/renderer/src/s_device.h`
- `src/libs/renderer/src/s_device.cpp`
- D3D9 conversion helpers

Acceptance criteria:

- Neutral `ClearFlags`, `PrimitiveType`, and `IndexFormat` can drive existing D3D9 calls.
- Legacy D3D9 APIs remain intact.
- At least one trivial internal renderer call path uses the neutral overload.

### PR 3: First low-risk caller migration

Files likely touched:

- one isolated debug or UI rendering helper
- renderer neutral API/conversion helpers as needed

Acceptance criteria:

- The chosen caller no longer references D3D primitive constants directly.
- D3D9 output/build remains unchanged.
- Remaining D3D references are not mechanically churned.

### PR 4: Config/editor backend adapter plan implementation

Files likely touched:

- `conanfile.py`
- `src/libs/editor/CMakeLists.txt`
- `src/libs/editor/src/storm/editor/engine_editor.cpp`
- `src/apps/configapp/src/CMakeLists.txt`
- `src/apps/configapp/src/configapp.cpp`

Acceptance criteria:

- ImGui backend selection no longer assumes DX9 everywhere.
- D3D9 path remains available.
- OpenGL ImGui backend can be introduced without changing engine renderer internals.

## Tracking metrics

Track these periodically to avoid losing sight of progress:

- total `VDX9RENDER` references
- total `D3D*` constant/type references outside `src/libs/renderer`
- total `IDirect3D*` references outside `src/libs/renderer`
- number of non-renderer files including `<d3d9.h>` or `<d3dx9.h>`
- number of callers using raw `GetD3DDevice()` or raw D3D texture/surface/buffer APIs
- number of public renderer APIs that still expose resource IDs as raw `int32_t`
- number of non-renderer/resource-owner fields storing texture, vertex buffer, index buffer, or render target IDs as raw integers
- number of technique/effect files with backend-specific assumptions

The useful trend is not immediate zero. The useful trend is that backend-specific references move inward toward `src/libs/renderer/src/backends/d3d9/` and out of public engine-facing headers.

## Definition of done for the abstraction layer

The abstraction work is complete enough to start serious OpenGL/WebGPU implementation when:

- General gameplay/UI modules can render common geometry without including D3D headers.
- The public renderer service exposes neutral handles and enums for common operations.
- Raw D3D access is isolated to renderer backend/compatibility code.
- D3D9 still works as a backend behind the neutral frontend.
- Build configuration can select a backend without requiring every backend's dependencies.
- The technique/shader plan is explicit, even if not all techniques have been migrated.

## References

- `docs/architecture.md`
- `docs/project-structure.md`
- `docs/services.md`
- `src/libs/renderer/include/dx9render.h`
- `src/libs/renderer/src/s_device.cpp`
- `src/libs/renderer/src/s_device.h`
- `cmake/linux.cmake`
