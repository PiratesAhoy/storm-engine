# Libs Components Guide

This repo’s reusable engine components live under `src/libs`. Each component is its own CMake target configured by the `STORM_SETUP` macro (defined in `cmake/StormSetup.cmake`). The top-level `src/libs/CMakeLists.txt` calls `add_all_subdirectories()`, so any new folder you add under `src/libs` is automatically picked up.

## Standard layout

Components follow this layout (all folders optional except `CMakeLists.txt`):

```
src/libs/<component>/
  CMakeLists.txt
  include/          # public headers (installed + added to include dirs)
  src/              # .cpp/.h implementation files
  testsuite/        # unit tests (optional)
  rsrc/             # resources (optional)
```

Global defaults are defined in `CMakeLists.txt` at the repo root:

- `include/` is the public header root
- `src/` is the source root
- `testsuite/` is the test root
- `rsrc/` is the resource root

`STORM_SETUP` globs these folders, so you do **not** list files manually.

## Component types

`STORM_SETUP` supports a few target types:

- `library`: a normal static library (most common for shared engine code).
- `storm_module`: a module library that is force-linked into the main executable (so its symbols are preserved).
- `executable`: not used under `src/libs`, but supported by the macro.

Pick `storm_module` when the component must be linked into `engine.exe` even if not directly referenced (e.g., runtime-registered systems).

## Minimal `CMakeLists.txt`

Basic library:

```
STORM_SETUP(
    TARGET_NAME my_component
    TYPE library
    DEPENDENCIES core util
)
```

Module with tests:

```
STORM_SETUP(
    TARGET_NAME my_module
    TYPE storm_module
    DEPENDENCIES core renderer
    TEST_DEPENDENCIES Catch2::Catch2WithMain
)
```

Notes:
- `TARGET_NAME` is usually the same as the folder name (lowercase, snake_case).
- If the component has only headers (no `src/` files), `STORM_SETUP` creates an `INTERFACE` library automatically.
- If there are **no** source or header files, `STORM_SETUP` prints a warning and skips the target.

## How to add a new component

1. Create a new folder under `src/libs/<component_name>`.
2. Add `CMakeLists.txt` with a `STORM_SETUP` block.
3. Add `include/` for public headers and `src/` for implementation.
4. (Optional) Add `testsuite/` and list `TEST_DEPENDENCIES`.
5. (Optional) Add `rsrc/` for component resources.

No parent CMake updates are required; `src/libs/CMakeLists.txt` discovers new subfolders automatically.

## Dependency conventions

- Internal engine libs are referenced by target name (usually the folder name).
- External dependencies are exposed via CMake targets (e.g., `tomlplusplus::tomlplusplus`, `nlohmann_json::nlohmann_json`) or variables like `${SDL2_LIBRARIES}`.
- Keep dependencies tight to avoid circular links; prefer depending on lower-level libs like `core`, `util`, `math`.
