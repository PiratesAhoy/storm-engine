#pragma once

#include <cstdint>

namespace storm::render
{

enum class RenderBackendType : uint8_t
{
    D3D9,
    OpenGL,
    WebGPU,
};

enum class PrimitiveType : uint8_t
{
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

enum class IndexFormat : uint8_t
{
    UInt16,
    UInt32,
};

enum class TextureFormat : uint8_t
{
    Unknown,
    A8R8G8B8,
    X8R8G8B8,
    R5G6B5,
    A1R5G5B5,
    A4R4G4B4,
    D16,
    D24S8,
};

enum class TextureUsage : uint8_t
{
    Sampled,
    RenderTarget,
    DepthStencil,
    Dynamic,
};

enum class BufferUsage : uint8_t
{
    Static,
    Dynamic,
    Stream,
};

enum class BufferType : uint8_t
{
    Vertex,
    Index,
};

enum class TransformType : uint8_t
{
    World,
    View,
    Projection,
};

enum ClearFlags : uint32_t
{
    ClearColor = 1u << 0u,
    ClearDepth = 1u << 1u,
    ClearStencil = 1u << 2u,
};

struct Rect
{
    int32_t left = 0;
    int32_t top = 0;
    int32_t right = 0;
    int32_t bottom = 0;
};

struct Viewport
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct Color
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

} // namespace storm::render
