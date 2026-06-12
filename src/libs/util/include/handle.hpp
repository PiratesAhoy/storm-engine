#pragma once

#include <cstdint>
#include <limits>
#include <type_traits>

namespace storm
{

template <typename Tag, typename HandleType = uint32_t>
class Handle
{
    static_assert(std::is_integral<HandleType>::value && std::is_unsigned<HandleType>::value,
                  "Handle value type must be an unsigned integer");

  public:
    using tag_type = Tag;
    using value_type = HandleType;

    constexpr Handle() noexcept = default;

    explicit constexpr Handle(value_type value) noexcept : value_(value)
    {
    }

    [[nodiscard]] static constexpr Handle FromValue(value_type value) noexcept
    {
        return Handle(value);
    }

    [[nodiscard]] constexpr value_type Value() const noexcept
    {
        return value_;
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value_ != invalidValue_;
    }

    constexpr Handle& Invalidate() noexcept
    {
        value_ = invalidValue_;
        return *this;
    }

    friend constexpr bool operator==(Handle lhs, Handle rhs) noexcept = default;

    static constexpr value_type InvalidValue() noexcept
    {
        return invalidValue_;
    }

    static constexpr Handle Invalid() noexcept
    {
        return Handle(invalidValue_);
    }

  private:
    value_type value_{invalidValue_};

    static constexpr value_type invalidValue_ = std::numeric_limits<value_type>::max();
};

} // namespace storm
