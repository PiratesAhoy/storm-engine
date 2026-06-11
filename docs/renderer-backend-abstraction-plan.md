# Renderer backend abstraction plan

This document describes a staged plan for abstracting the current DirectX 9 renderer so the engine can later support other rendering backends such as OpenGL or WebGPU.

The goal is not to replace the renderer in one large rewrite. The goal is to first create an API-neutral renderer boundary while keeping the current D3D9 renderer working, then move implementation details behind backend interfaces, and only then add new backends.

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
- Non-Windows builds still rely on native D3D9-compatible APIs through Gallium Nine or DXVK Native (`cmake/linux.cmake`) rather than an API-neutral renderer.

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
  render_types.h
  render_handles.h
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

Suggested initial header: `src/libs/renderer/include/render/render_types.h`.

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
- `BufferHandle`
- `RenderTargetHandle`
- `ShaderHandle` / `ProgramHandle` later
- `FontHandle` can remain higher-level if font rendering stays in the frontend

Handles should be opaque integer or strong typedef-style values owned by the renderer frontend. They should not expose backend object pointers.

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

### Phase 1: Add neutral renderer vocabulary

Purpose: create shared language without changing behavior.

Tasks:

- Add neutral renderer type headers under `src/libs/renderer/include/render/`.
- Add D3D9 conversion helpers under `src/libs/renderer/src/backends/d3d9/` or a temporary internal file.
- Add unit tests for pure conversion functions where practical.
- Keep all D3D9 includes out of neutral public headers.

Suggested first conversions:

- `PrimitiveType` -> `D3DPRIMITIVETYPE`
- `IndexFormat` -> `D3DFORMAT`
- `TransformType` -> `D3DTRANSFORMSTATETYPE`
- `ClearFlags` -> `D3DCLEAR_*`
- `TextureFormat` -> `D3DFORMAT` for the subset currently used

Validation:

- Include neutral headers from a non-renderer module without pulling in `<d3d9.h>`.
- Existing D3D9 renderer build still passes.

### Phase 2: Add neutral draw/resource APIs beside legacy APIs

Purpose: prove neutral types can drive the current renderer.

Tasks:

- Add neutral overloads to the renderer service/frontend for the smallest useful draw/resource paths.
- Implement each neutral overload by mapping to the current D3D9 implementation.
- Keep legacy methods unchanged.

Good first APIs:

- clear flags
- viewport get/set
- `DrawPrimitiveUP`
- `DrawIndexedPrimitiveUP`
- texture set by existing texture id
- simple vertex/index buffer create/lock/unlock wrappers

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

### Phase 3: Convert low-risk call sites to neutral APIs

Purpose: reduce D3D9 leakage outside the renderer without changing the backend.

Start with simple rendering helpers and debug/UI paths, not major scene systems.

Candidate areas:

- debug lines/vectors
- simple sprite/rect drawing
- small UI helpers in `battle_interface` or `xinterface`
- isolated helper functions that currently pass `D3DPT_*`, `D3DFVF_*`, or clear flags

Tasks:

- Replace D3D constants with neutral enums in selected call sites.
- Replace raw D3D structs with neutral structs where the mapping is clear.
- Keep changes small enough to review subsystem by subsystem.

Validation:

- Build after each subsystem conversion.
- Run any available UI/render smoke test.
- Track remaining D3D usages outside `src/libs/renderer`.

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
- Keep texture/font/resource id management in the frontend unless there is a strong reason to move it.
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
- `updateBuffer(...)`
- `draw(...)`
- `drawIndexed(...)`

Validation:

- D3D9 backend produces equivalent behavior.
- Frontend owns no raw `IDirect3D9`/`IDirect3DDevice9` fields after the moved subset.
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
- Shader constants/registers vs uniforms/bind groups.
- Render state mutability vs pipeline object models, especially for WebGPU.
- Lock/unlock buffer and texture APIs vs mapped/staging/update APIs.

## Suggested first implementation PRs

### PR 1: Neutral render type scaffolding

Files likely touched:

- add `src/libs/renderer/include/render/render_types.h`
- add `src/libs/renderer/include/render/render_handles.h`
- add internal D3D9 conversion helpers under `src/libs/renderer/src/`
- add small conversion tests if the test structure supports it

Acceptance criteria:

- Neutral headers do not include `<d3d9.h>`.
- D3D9 renderer still builds.
- No gameplay call site migration yet unless needed for compile coverage.

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
