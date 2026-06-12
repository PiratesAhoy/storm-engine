#pragma once

#include <handle.hpp>

namespace storm::render
{

using TextureHandle = storm::Handle<struct TextureHandleTag>;
using VertexBufferHandle = storm::Handle<struct VertexBufferHandleTag>;
using IndexBufferHandle = storm::Handle<struct IndexBufferHandleTag>;
using BufferHandle = storm::Handle<struct BufferHandleTag>;
using RenderTargetHandle = storm::Handle<struct RenderTargetHandleTag>;
using ShaderHandle = storm::Handle<struct ShaderHandleTag>;
using ProgramHandle = storm::Handle<struct ProgramHandleTag>;

} // namespace storm::render
