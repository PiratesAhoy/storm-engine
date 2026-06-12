#include "handle.hpp"

#include <catch2/catch_all.hpp>

#include <cstring>
#include <limits>
#include <type_traits>
#include <utility>

namespace
{

struct VertexBufferTag;
struct IndexBufferTag;

using VertexBufferHandle = storm::Handle<VertexBufferTag>;
using IndexBufferHandle = storm::Handle<IndexBufferTag>;
using WideHandle = storm::Handle<struct WideHandleTag, uint64_t>;

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

static_assert(sizeof(VertexBufferHandle) == sizeof(uint32_t));
static_assert(alignof(VertexBufferHandle) == alignof(uint32_t));
static_assert(sizeof(WideHandle) == sizeof(uint64_t));
static_assert(alignof(WideHandle) == alignof(uint64_t));

static_assert(std::is_trivially_copyable<VertexBufferHandle>::value);
static_assert(std::is_standard_layout<VertexBufferHandle>::value);

static_assert(std::is_constructible<VertexBufferHandle, uint32_t>::value);
static_assert(!std::is_convertible<uint32_t, VertexBufferHandle>::value);
static_assert(!std::is_convertible<VertexBufferHandle, uint32_t>::value);

static_assert(is_equality_comparable<VertexBufferHandle, VertexBufferHandle>::value);
static_assert(!is_equality_comparable<VertexBufferHandle, IndexBufferHandle>::value);
static_assert(!is_equality_comparable<VertexBufferHandle, uint32_t>::value);
static_assert(!is_equality_comparable<uint32_t, VertexBufferHandle>::value);

static_assert(!is_less_than_comparable<VertexBufferHandle, VertexBufferHandle>::value);
static_assert(!is_less_than_comparable<VertexBufferHandle, IndexBufferHandle>::value);
static_assert(!is_less_than_comparable<VertexBufferHandle, uint32_t>::value);
static_assert(!is_less_than_comparable<uint32_t, VertexBufferHandle>::value);

static_assert(!VertexBufferHandle{}.IsValid());
static_assert(VertexBufferHandle::FromValue(0).IsValid());
static_assert(VertexBufferHandle::FromValue(42).IsValid());
static_assert(VertexBufferHandle::InvalidValue() == std::numeric_limits<uint32_t>::max());
static_assert(!VertexBufferHandle::FromValue(VertexBufferHandle::InvalidValue()).IsValid());
static_assert(!VertexBufferHandle::Invalid().IsValid());
static_assert(VertexBufferHandle::Invalid().Value() == VertexBufferHandle::InvalidValue());

} // namespace

TEST_CASE("Strongly typed handles keep integer layout without integer comparisons", "[utils]")
{
    const auto vertex_buffer = VertexBufferHandle::FromValue(42);
    const auto same_vertex_buffer = VertexBufferHandle::FromValue(42);
    const auto other_vertex_buffer = VertexBufferHandle::FromValue(7);

    CHECK(vertex_buffer == same_vertex_buffer);
    CHECK(vertex_buffer != other_vertex_buffer);
    CHECK(vertex_buffer.Value() == 42);

    uint32_t raw_value = 0;
    std::memcpy(&raw_value, &vertex_buffer, sizeof(raw_value));

    CHECK(raw_value == 42);
}

TEST_CASE("Strongly typed handles use the maximum integer value as the invalid sentinel", "[utils]")
{
    SECTION("Default constructed handles are invalid")
    {
        const VertexBufferHandle handle;

        CHECK_FALSE(handle.IsValid());
        CHECK(handle.Value() == VertexBufferHandle::InvalidValue());
    }

    SECTION("Zero is a valid handle value")
    {
        const auto handle = VertexBufferHandle::FromValue(0);

        CHECK(handle.IsValid());
        CHECK(handle.Value() == 0);
    }

    SECTION("Explicitly constructed handles are valid unless they use the invalid sentinel")
    {
        const auto valid_handle = VertexBufferHandle::FromValue(42);
        const auto invalid_handle = VertexBufferHandle::FromValue(VertexBufferHandle::InvalidValue());

        CHECK(valid_handle.IsValid());
        CHECK_FALSE(invalid_handle.IsValid());
    }

    SECTION("Invalid factory creates an invalid handle")
    {
        const auto handle = VertexBufferHandle::Invalid();

        CHECK_FALSE(handle.IsValid());
        CHECK(handle.Value() == VertexBufferHandle::InvalidValue());
        CHECK(handle == VertexBufferHandle::FromValue(VertexBufferHandle::InvalidValue()));
    }

    SECTION("Invalidate marks an existing handle invalid and returns the same handle")
    {
        auto handle = VertexBufferHandle::FromValue(42);
        auto &invalidated_handle = handle.Invalidate();

        CHECK(&invalidated_handle == &handle);
        CHECK_FALSE(handle.IsValid());
        CHECK(handle.Value() == VertexBufferHandle::InvalidValue());
    }
}
