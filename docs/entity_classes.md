# Entity Classes

This document summarizes how `Entity` classes are defined, registered, created, and updated in the engine.

## Overview

- `Entity` is the abstract base for game/runtime objects. Every concrete entity must implement `Init()` and `ProcessStage()`.
- Entities are created via the class registry (`CREATE_CLASS`) and managed by `EntityManager`.
- The engine drives entities through layer-based update stages (`execute` and `realize`).

See: `src/libs/core/include/entity.h`, `src/libs/core/include/vma.hpp`, `src/libs/core/src/entity_manager.cpp`, `src/libs/core/src/core_impl.cpp`.

## Implementing an Entity

At minimum, implement the base interface:

```cpp
class MyEntity : public Entity {
public:
    bool Init() override;
    void ProcessStage(Stage stage, uint32_t delta) override;
};
```

Common optional overrides:

- `ProcessMessage(MESSAGE&)` for script/event messages.
- `AttributeChanged(ATTRIBUTES*)` if you react to attribute updates.
- `ShowEditor()` to expose custom debug UI (ImGui).

The base class also exposes:

- `GetId()` to retrieve the entity ID.
- `AttributesPointer` for attribute access (assigned by the creator).

See: `src/libs/core/include/entity.h`, `src/libs/core/src/entity.cpp`.

## Registering the Class

Each concrete entity must be registered so the engine can create it by name.

In a `.cpp` file:

```cpp
CREATE_CLASS(MyEntity)
```

The macro installs a `VMA` descriptor in the global registry and makes the entity constructible by name (the string is the class name used in the macro).

See: `src/libs/core/include/vma.hpp` and usages like `src/libs/sailors/src/sailors.cpp`.

## Creation Path

Entities are created by name through `EntityManager::CreateEntity` (reachable via `CoreImpl::CreateEntity`).

High-level flow:

1. Look up a `VMA` in `__STORM_CLASSES_REGISTRY` by hash + name.
2. Call `VMA::CreateClass()` to allocate the entity.
3. Assign an ID and `AttributesPointer`.
4. Call `Init()`; on failure the entity is marked for deletion.

See: `src/libs/core/src/entity_manager.cpp` and `src/libs/core/src/core_impl.cpp`.

## Entity IDs

Entity IDs are `uint64_t` values built from a monotonically increasing stamp (high 32 bits) plus an index into the entity storage (low 32 bits). This allows reuse of storage slots while still detecting stale IDs.

See: `src/libs/core/src/entity_manager.cpp` (`CalculateEntityId`).

## Update Stages and Layers

Entities are processed via layers, which are tagged as `execute` or `realize`.

- The engine collects entity IDs in all layers of a given type.
- For each entity ID, `ProcessStage(Stage::execute, delta)` or `ProcessStage(Stage::realize, delta)` is called.

Layers can be frozen; frozen layers are skipped. Entities can belong to multiple layers (potentially multiple times in the same type).

See: `src/libs/core/src/core_impl.cpp` and `src/libs/core/src/entity_manager.cpp`.

## Adding and Removing from Layers

Use `CoreImpl::AddToLayer` / `RemoveFromLayer` to place entities into update layers with a priority. The layer list is kept sorted by priority, so execution order is stable within a layer.

See: `src/libs/core/src/entity_manager.cpp` and `src/libs/core/src/core_impl.cpp`.

## Deletion Lifecycle

`EraseEntity` marks an entity for deletion. Actual destruction is deferred until `EntityManager::NewLifecycle()`, which removes it from layers, deletes the object, and recycles the storage slot.

See: `src/libs/core/src/entity_manager.cpp`.

## Attributes and Messaging

Entities can receive messages and expose attributes:

- `ProcessMessage(MESSAGE&)` is called by `CoreImpl::Send_Message`.
- `AttributesPointer` is set on creation and used by helper functions in `CoreImpl` to get/set attributes.
- `AttributeChanged(ATTRIBUTES*)` is called when attributes are updated via Core.

See: `src/libs/core/include/entity.h` and `src/libs/core/src/core_impl.cpp`.

## Minimal Example

```cpp
// my_entity.h
class MyEntity : public Entity {
public:
    bool Init() override { return true; }

    void ProcessStage(Stage stage, uint32_t delta) override {
        switch (stage) {
        case Stage::execute:
            // update logic
            break;
        case Stage::realize:
            // render logic
            break;
        default:
            break;
        }
    }
};

// my_entity.cpp
#include "my_entity.h"
CREATE_CLASS(MyEntity)
```
