#include <renderer/render_handles.hpp>
#include <renderer/render_types.hpp>

#include <catch2/catch_all.hpp>

#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{

template <typename...>
using void_t = void;

template <typename Left, typename Right, typename = void>
struct is_equality_comparable : std::false_type
{
};

template <typename Left, typename Right>
struct is_equality_comparable<Left, Right, void_t<decltype(std::declval<Left>() == std::declval<Right>())>>
    : std::true_type
{
};

template <typename Left, typename Right, typename = void>
struct is_less_than_comparable : std::false_type
{
};

template <typename Left, typename Right>
struct is_less_than_comparable<Left, Right, void_t<decltype(std::declval<Left>() < std::declval<Right>())>>
    : std::true_type
{
};

static_assert(sizeof(storm::render::TextureHandle) == sizeof(uint32_t));
static_assert(alignof(storm::render::TextureHandle) == alignof(uint32_t));
static_assert(std::is_trivially_copyable<storm::render::TextureHandle>::value);
static_assert(std::is_standard_layout<storm::render::TextureHandle>::value);

static_assert(std::is_constructible<storm::render::TextureHandle, uint32_t>::value);
static_assert(!std::is_convertible<uint32_t, storm::render::TextureHandle>::value);
static_assert(!std::is_convertible<storm::render::TextureHandle, uint32_t>::value);

static_assert(is_equality_comparable<storm::render::TextureHandle, storm::render::TextureHandle>::value);
static_assert(!is_equality_comparable<storm::render::TextureHandle, storm::render::VertexBufferHandle>::value);
static_assert(!is_equality_comparable<storm::render::TextureHandle, uint32_t>::value);
static_assert(!is_equality_comparable<uint32_t, storm::render::TextureHandle>::value);

static_assert(!is_less_than_comparable<storm::render::TextureHandle, storm::render::TextureHandle>::value);
static_assert(!is_less_than_comparable<storm::render::TextureHandle, storm::render::VertexBufferHandle>::value);
static_assert(!is_less_than_comparable<storm::render::TextureHandle, uint32_t>::value);
static_assert(!is_less_than_comparable<uint32_t, storm::render::TextureHandle>::value);

static_assert(!storm::render::TextureHandle{}.IsValid());
static_assert(storm::render::TextureHandle::FromValue(0).IsValid());
static_assert(storm::render::TextureHandle::FromValue(42).IsValid());
static_assert(storm::render::TextureHandle::InvalidValue() == std::numeric_limits<uint32_t>::max());
static_assert(!storm::render::TextureHandle::Invalid().IsValid());
static_assert(storm::render::HandleFromLegacyId<storm::render::TextureHandle>(-1) == storm::render::TextureHandle::Invalid());
static_assert(storm::render::HandleFromLegacyId<storm::render::TextureHandle>(0).IsValid());
static_assert(storm::render::HandleFromLegacyId<storm::render::TextureHandle>(0).Value() == 0);
static_assert(storm::render::HandleToLegacyId(storm::render::TextureHandle::Invalid()) == -1);
static_assert(storm::render::HandleToLegacyId(storm::render::TextureHandle::FromValue(0)) == 0);
static_assert(static_cast<uint32_t>(storm::render::PrimitiveType::TriangleList) == 0);
static_assert(static_cast<uint32_t>(storm::render::IndexFormat::UInt32) == 1);
static_assert(storm::render::ClearColor != storm::render::ClearDepth);

} // namespace

TEST_CASE("Renderer resource handles are strongly typed frontend tokens", "[renderer]")
{
    const auto texture = storm::render::TextureHandle::FromValue(0);
    const auto same_texture = storm::render::TextureHandle::FromValue(0);
    const auto other_texture = storm::render::TextureHandle::FromValue(7);

    CHECK(texture.IsValid());
    CHECK(texture == same_texture);
    CHECK(texture != other_texture);
    CHECK(texture.Value() == 0);
}

TEST_CASE("Renderer handle domains have independent invalid sentinels", "[renderer]")
{
    const auto texture = storm::render::TextureHandle::Invalid();
    const auto vertex_buffer = storm::render::VertexBufferHandle::Invalid();
    const auto index_buffer = storm::render::IndexBufferHandle::Invalid();
    const auto buffer = storm::render::BufferHandle::Invalid();
    const auto render_target = storm::render::RenderTargetHandle::Invalid();
    const auto shader = storm::render::ShaderHandle::Invalid();
    const auto program = storm::render::ProgramHandle::Invalid();

    CHECK_FALSE(texture.IsValid());
    CHECK_FALSE(vertex_buffer.IsValid());
    CHECK_FALSE(index_buffer.IsValid());
    CHECK_FALSE(buffer.IsValid());
    CHECK_FALSE(render_target.IsValid());
    CHECK_FALSE(shader.IsValid());
    CHECK_FALSE(program.IsValid());
}

TEST_CASE("Renderer handles bridge legacy signed ids without losing slot zero", "[renderer]")
{
    const auto invalid_vertex_buffer = storm::render::HandleFromLegacyId<storm::render::VertexBufferHandle>(-1);
    const auto slot_zero_vertex_buffer = storm::render::HandleFromLegacyId<storm::render::VertexBufferHandle>(0);
    const auto slot_seven_index_buffer = storm::render::HandleFromLegacyId<storm::render::IndexBufferHandle>(7);

    CHECK_FALSE(invalid_vertex_buffer.IsValid());
    CHECK(slot_zero_vertex_buffer.IsValid());
    CHECK(slot_zero_vertex_buffer.Value() == 0);
    CHECK(slot_seven_index_buffer.IsValid());
    CHECK(slot_seven_index_buffer.Value() == 7);

    CHECK(storm::render::HandleToLegacyId(invalid_vertex_buffer) == -1);
    CHECK(storm::render::HandleToLegacyId(slot_zero_vertex_buffer) == 0);
    CHECK(storm::render::HandleToLegacyId(slot_seven_index_buffer) == 7);
}
