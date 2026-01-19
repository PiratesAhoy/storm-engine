#pragma once

#include "core.h"
#include "dx9render.h"
#include "entity.h"

#include <stdexcept>

// Base helper for entities that use the VDX9RENDER service.
// Caches the pointer but re-validates on each access.
class EntityWithRenderer : public Entity
{
  protected:
    VDX9RENDER *Renderer()
    {
        if (renderer_ == nullptr)
        {
            renderer_ = core.GetServiceX<VDX9RENDER>();
            if (renderer_ == nullptr)
            {
                throw std::runtime_error("EntityWithRenderer: VDX9RENDER service not available");
            }
        }
        return renderer_;
    }

    [[nodiscard]] bool HasRenderer()
    {
        return Renderer() != nullptr;
    }

  private:
    VDX9RENDER *renderer_ = nullptr;
};
