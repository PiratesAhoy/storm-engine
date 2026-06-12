# Coding guidelines

These guidelines capture conventions that are already visible in the repository history. They complement `CONTRIBUTING.md`, `.clang-format`, and the architecture/service documentation.

## Evidence base

The recent history shows several recurring maintenance themes:

- service registration and lookup cleanup: `c0e40f9e`, `9471221d`, `58b25d5d`
- crash prevention around missing script data, messages, entities, and assets: `ff99b8a0`, `67496261`, `7991fde4`, `3b45926b`, `f6e9d6e1`, `c195bb48`, `4dbc2bce`, `18fe7b7c`
- build and dependency maintenance through CMake, Conan, and CI: `ccdba08e`, `decf7d2b`, `ffad2a25`, `e2b7a2b9`, `e4cce9e5`
- configuration and testable utility refactors: `b964b1a2`, `eafc8364`, `21e2a4bc`, `84c5b49c`
- architecture documentation and renderer abstraction planning: `92b70850`

## Formatting and basic C++ style

- Format C++ with the root `.clang-format` file. It is based on Microsoft style, uses 4-space indentation, 120-column line width, right-aligned pointers, sorted includes, and no tabs.
- Keep generated or mechanical formatting separate from behavior changes when possible. History favors small, understandable commits and PRs.
- Use C++20 facilities already present in the codebase when they simplify ownership, optional data, filesystem paths, string views, or containers.
- Match the style of the file being edited. This codebase contains older engine code and newer namespaced code; avoid broad style rewrites in unrelated areas.
- Prefer clear names over abbreviations in new code, but do not rename legacy identifiers unless the change is part of the task.

## Safety and error handling

- Treat script data, game assets, messages, and optional entities as unreliable inputs. Check for missing values before dereferencing them.
- Prefer a safe default or an early return over a crash when the engine can continue. Recent fixes default missing weapon IDs, skip unsupported sky texture formats, tolerate missing `Render` script data, and handle absent `engine.ini`.
- When a failure is useful to diagnose, log a specific message with the path, format, entity, or script context that failed. Avoid vague errors.
- Do not cache pointers to entities whose lifetime can change across scene, time-of-day, or reload transitions. Re-acquire them when used, or pass a local reference through the call path after a null check.
- Use `core.GetEntityPointerSafe(...)` where the entity may be absent. If using `GetEntityPointer(...)`, make the required lifetime/availability explicit.
- Validate message formats at the boundary where messages are decoded. Convert validation failures into script/compiler errors instead of allowing generic exceptions to unwind into crashes.
- Keep compatibility behavior intact unless a change explicitly targets it. Add guards and adapters before removing legacy paths.

## Services and engine architecture

- A service should have one concrete class that inherits from `SERVICE` and is registered with `CREATE_SERVICE(...)`.
- Define `constexpr static ServiceName` on the public virtual interface when one exists, otherwise on the concrete service class.
- Prefer `core.GetServiceX<ServiceType>()` over string-based `core.GetService("...")` calls in new or touched code.
- Keep service names stable unless the change includes a compatibility plan; names such as `dx9render`, `PCS_CONTROLS`, and `STRSERVICE` are part of the runtime integration surface.
- Do not expose backend-specific types from generic or higher-level headers. Renderer migration work should introduce narrow compatibility seams before large rewrites.
- Preserve the core/entity/service/module registration model when refactoring subsystems. CMake whole-archive behavior keeps `storm_module` registration objects alive, so target type changes are architectural changes.

## Build, dependencies, and tests

- Add dependencies through `conanfile.py`, top-level `find_package(...)`, and the relevant `STORM_SETUP(...)` `DEPENDENCIES` or `TEST_DEPENDENCIES` entry. Do not rely on incidental transitive linkage.
- Use `STORM_SETUP(...)` for engine targets. Put tests in a module's `testsuite` directory so the helper creates the `<target>-test` executable.
- Prefer Catch2 tests for isolated config, parser, and utility behavior. Existing examples live in `src/libs/config/testsuite`, `src/libs/util/testsuite`, and `src/libs/xinterface/testsuite`.
- For behavior fixes that can be isolated from full game assets, add or update tests. For asset/runtime-only fixes, document the scenario and keep the guard narrow.
- Keep Windows and non-Windows build behavior in mind. Renderer, DirectX compatibility, CI, crash reporting, and runtime binary deployment all have platform-specific paths.
- When changing output paths, dependency copying, or CI scripts, verify both debug/release and artifact expectations where practical.

## Configuration and data handling

- Prefer standard formats and libraries over custom formats when adding new configuration or data paths. The roadmap and history favor TOML/JSON, standard containers, and third-party libraries where they reduce maintenance.
- Preserve legacy format compatibility while introducing replacements. The config history keeps INI loading working while converting data to the newer internal representation.
- Keep configuration keys case-insensitive where existing config APIs promise that behavior.
- Use `std::filesystem::path` for paths and convert to strings only at API boundaries that require C-style or library-specific path strings.

## Renderer and asset work

- Do not widen DirectX 9 coupling. If renderer-facing code does not truly need a D3D type, introduce or use a neutral type at the boundary.
- Keep `VDX9RENDER` and `DX9RENDER` compatibility while adding backend-neutral seams. Rename only after callers no longer depend on the old surface.
- Treat textures and render resources as data-dependent. Check formats and resource availability before locking, reading, or rendering them.
- Keep shader/effect assets under `src/techniques`; CMake copies them into the runtime output through the `engine_techniques` target.
- For rendering migration changes, update the relevant docs: `docs/architecture.md`, `docs/project-structure.md`, and `docs/plans/renderer-backend-abstraction-plan.md`.

## Commit and PR hygiene

- Follow `CONTRIBUTING.md`: imperative present-tense commit subjects, first line under 72 characters, understandable names, and focused PRs.
- Prefer one logical change per commit. The history is easiest to follow where commits say exactly what changed, for example “Do not crash when ...”, “Fix ...”, “Add ...”, or “Refactor ...”.
- Include reasoning and a summary in PRs, especially for architecture, dependency, or compatibility changes.
- Avoid mixing unrelated cleanup with bug fixes. If a crash fix requires a narrow cleanup, keep it local to the failing path.
