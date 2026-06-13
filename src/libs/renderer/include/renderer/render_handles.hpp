#pragma once

#include <handle.hpp>

#include <cstdint>

namespace storm::render
{

using TextureHandle = storm::Handle<struct TextureHandleTag>;
using VertexBufferHandle = storm::Handle<struct VertexBufferHandleTag>;
using IndexBufferHandle = storm::Handle<struct IndexBufferHandleTag>;
using BufferHandle = storm::Handle<struct BufferHandleTag>;
using RenderTargetHandle = storm::Handle<struct RenderTargetHandleTag>;
using ShaderHandle = storm::Handle<struct ShaderHandleTag>;
using ProgramHandle = storm::Handle<struct ProgramHandleTag>;

template <typename Handle>
[[nodiscard]] constexpr Handle HandleFromLegacyId(int32_t id) noexcept
{
    if (id < 0)
    {
        return Handle::Invalid();
    }

    return Handle::FromValue(static_cast<typename Handle::value_type>(id));
}

template <typename Handle>
[[nodiscard]] constexpr int32_t HandleToLegacyId(Handle handle) noexcept
{
    if (!handle.IsValid())
    {
        return -1;
    }

    return static_cast<int32_t>(handle.Value());
}

} // namespace storm::render
