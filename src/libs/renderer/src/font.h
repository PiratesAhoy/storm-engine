#pragma once

#include "dx9render.h"
#include "renderer/render_handles.hpp"

#include <string_view>

namespace storm {

struct FontPrintOverrides
{
    std::optional<float> scale;
    std::optional<uint32_t> color;
    std::optional<bool> shadow;
};

class VFont
{
  public:
    virtual ~VFont() = default;

    [[nodiscard]] virtual std::optional<size_t> Print(float x, float y, const std::string_view &text,
                                                      const FontPrintOverrides &overrides = {}) = 0;

    [[nodiscard]] virtual size_t GetStringWidth(const std::string_view &text,
                                                const FontPrintOverrides &overrides = {}) const = 0;

    [[nodiscard]] virtual size_t GetHeight() const = 0;

    virtual void TempUnload() = 0;
    virtual void RepeatInit() = 0;
};

} // namespace storm

struct FLOAT_RECT
{
    float x1, y1, x2, y2;
};

struct FONT_SYMBOL
{
    FLOAT_RECT Pos;
    FLOAT_RECT Tuv;
};

class FONT final : public storm::VFont
{
  public:
    FONT(VDX9RENDER &renderer, IDirect3DDevice9 &device);
    ~FONT() override;
    bool Init(const char *font_name, const char *iniName);
    void TempUnload() override;
    void RepeatInit() override;

    std::optional<size_t> Print(float x, float y, const std::string_view &text,
                                const storm::FontPrintOverrides &overrides = {}) override;

    [[nodiscard]] size_t GetStringWidth(const std::string_view &text,
                                        const storm::FontPrintOverrides &overrides = {}) const override;

    [[nodiscard]] size_t GetHeight() const override
    {
        return height_;
    }

  private:
    int32_t UpdateVertexBuffer(int32_t x, int32_t y, const char *data_PTR, int utf8length, float scale, uint32_t color);

    std::vector<FONT_SYMBOL> charDescriptors_{};

    std::string techniqueName_{};
    std::string textureName_{};

    VDX9RENDER &renderService_;
    IDirect3DDevice9 &device_;
    IDirect3DVertexBuffer9 *vertexBuffer_ = nullptr;

    float scale_ = 1;
    float horizontalAspectRatio_ = 1;
    float verticalAspectRatio_ = 1;

    uint32_t color_ = 0xffffffff;

    storm::render::TextureHandle textureHandle_;
    int32_t height_ = 0;
    int32_t textureSizeX_ = 1;
    int32_t textureSizeY_ = 1;
    int32_t symbolInterval_ = 0;

    int32_t shadowOffsetX_ = 2;
    int32_t shadowOffsetY_ = 2;

    int32_t spacebarWidth_ = 8;

    bool drawShadows_ = false;

    bool initialized_ = false;
};
