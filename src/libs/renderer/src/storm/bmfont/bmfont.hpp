#pragma once

#include "../../font.h"

#include <cstdint>
#include <string>
#include <vector>

namespace storm::bmfont {

class BmCharacter {
  public:
    char32_t id{};
    int16_t x{};
    int16_t y{};
    uint16_t width{};
    uint16_t height{};
    int16_t xoffset{};
    int16_t yoffset{};
    int16_t xadvance{};
    uint8_t page{};
    uint8_t channel{};
};

class BmKerning {
  public:
    char32_t first{};
    char32_t second{};
    int16_t amount{};
};

class BmTexture {
  public:
    std::string texturePath_;
    int32_t textureHandle_ = -1;
};

class BmFont : public VFont {
  public:
    BmFont(const std::string_view &file_path, VDX9RENDER &renderer);
    ~BmFont() override = default;

    std::optional<size_t> Print(float x, float y, const std::string_view &text,
                                const FontPrintOverrides &overrides) override;
    size_t GetStringWidth(const std::string_view &text, const FontPrintOverrides &overrides) const override;
    size_t GetHeight() const override;
    void TempUnload() override;
    void RepeatInit() override;

  private:
    void LoadFromFnt(const std::string &file_path);

    void InitTextures();
    void InitVertexBuffer();

    struct UpdateVertexBufferResult {
        size_t characters{};
        float xoffset{};
    };

    UpdateVertexBufferResult UpdateVertexBuffer(float x, float y, const std::string_view &text, float scale, uint32_t color);

    const BmCharacter *GetCharacter(int32_t id) const;

    std::vector<BmCharacter> characters_;
    std::vector<BmKerning> kerning_;
    std::vector<BmTexture> textures_;

    VDX9RENDER &renderer_;

    size_t lineHeight_{};
    size_t base_{};
    size_t textureWidth_{};
    size_t textureHeight_{};

    int32_t vertexBuffer_{};
};

} // namespace storm::bmfont