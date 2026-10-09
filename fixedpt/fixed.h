#pragma once

#include <cassert>
#include <cmath>
#include <compare>
#include <cstdint>
#include <limits>

class Fixed16;
class FixedF;
class FixedI;

namespace detail
{

[[nodiscard]]
constexpr std::int32_t floatToFixed(float value, int fractional_bits) noexcept
{
    return static_cast<std::int32_t>(value * static_cast<float>(1 << fractional_bits));
}

[[nodiscard]]
constexpr std::int32_t intToFixed(std::int32_t value, int fractional_bits) noexcept
{
    return value << fractional_bits;
}

[[nodiscard]]
constexpr float fixedToFloat(std::int32_t value, int fractional_bits) noexcept
{
    return static_cast<float>(value) / static_cast<float>(1 << fractional_bits);
}

[[nodiscard]]
constexpr std::int32_t fixedToInt(std::int32_t value, int fractional_bits) noexcept
{
    return value >> fractional_bits;
}

} // namespace detail


// Fixed-point number with P fractional bits in a signed 32-bit container (Q(32-P).P).
template <int P>
    requires(P > 0 && P < 32)
class Fixed
{
public:
    static constexpr int PRECISION = P;

    constexpr Fixed() noexcept = default;

    explicit Fixed(float value) noexcept;
    explicit Fixed(double value) noexcept;
    explicit constexpr Fixed(std::int32_t value) noexcept;
    constexpr Fixed(std::int32_t numerator, std::int32_t denominator) noexcept;

    Fixed& operator=(float value) noexcept;
    Fixed& operator=(double value) noexcept;
    constexpr Fixed& operator=(std::int32_t value) noexcept;

    [[nodiscard]]
    constexpr bool operator==(const Fixed& other) const noexcept = default;

    [[nodiscard]]
    constexpr std::strong_ordering operator<=>(const Fixed& other) const noexcept = default;

    [[nodiscard]] constexpr bool operator>(std::int32_t value) const noexcept;
    [[nodiscard]] constexpr bool operator<(std::int32_t value) const noexcept;
    [[nodiscard]] constexpr bool operator>=(std::int32_t value) const noexcept;
    [[nodiscard]] constexpr bool operator<=(std::int32_t value) const noexcept;
    [[nodiscard]] constexpr bool operator==(std::int32_t value) const noexcept;
    [[nodiscard]] constexpr bool operator!=(std::int32_t value) const noexcept;

    [[nodiscard]] constexpr Fixed operator+(Fixed other) const noexcept;
    [[nodiscard]] constexpr Fixed operator-(Fixed other) const noexcept;
    [[nodiscard]] constexpr Fixed operator*(std::int32_t value) const noexcept;
    [[nodiscard]] constexpr Fixed operator/(std::int32_t value) const noexcept;
    constexpr Fixed& operator+=(Fixed other) noexcept;
    constexpr Fixed& operator-=(Fixed other) noexcept;
    constexpr Fixed& operator*=(Fixed other) noexcept;
    [[nodiscard]] constexpr Fixed operator-() const noexcept;

    [[nodiscard]] constexpr Fixed abs() const noexcept;
    [[nodiscard]] constexpr Fixed square() const noexcept;
    [[nodiscard]] constexpr Fixed inverse() const noexcept;
    [[nodiscard]] constexpr Fixed sqrt() const noexcept;
    [[nodiscard]] constexpr Fixed invSqrt() const noexcept;
    [[nodiscard]] constexpr Fixed round() const noexcept;
    [[nodiscard]] constexpr Fixed floor() const noexcept;
    [[nodiscard]] constexpr Fixed ceil() const noexcept;

    [[nodiscard]] constexpr Fixed inverseApprox() const noexcept;
    [[nodiscard]] constexpr Fixed sqrtApprox() const noexcept;
    [[nodiscard]] constexpr Fixed invSqrtApprox() const noexcept;

    [[nodiscard]] Fixed sin() const noexcept;
    [[nodiscard]] Fixed cos() const noexcept;
    void sinCos(Fixed& sin_result, Fixed& cos_result) const noexcept;

    [[nodiscard]] Fixed sinApprox() const noexcept;
    [[nodiscard]] Fixed cosApprox() const noexcept;
    void sinCosApprox(Fixed& sin_result, Fixed& cos_result) const noexcept;

    [[nodiscard]] constexpr float getFloat() const noexcept;
    [[nodiscard]] constexpr std::int32_t getInt() const noexcept;

    [[nodiscard]]
    constexpr std::int32_t getRawValue() const noexcept
    {
        return x_;
    }

    constexpr void setRawValue(std::int32_t raw_value) noexcept
    {
        x_ = raw_value;
    }

    [[nodiscard]]
    static constexpr Fixed fromRaw(std::int32_t raw_value) noexcept
    {
        return Fixed(raw_value, RawTag{});
    }

    [[nodiscard]]
    static constexpr Fixed getMaxValue() noexcept
    {
        return fromRaw(std::numeric_limits<std::int32_t>::max());
    }

    [[nodiscard]]
    static constexpr Fixed getMinValue() noexcept
    {
        return fromRaw(std::numeric_limits<std::int32_t>::min());
    }

    [[nodiscard]]
    static constexpr Fixed getEpsilon() noexcept
    {
        return fromRaw(1);
    }

    [[nodiscard]]
    static constexpr std::int32_t getFractionalBits() noexcept
    {
        return PRECISION;
    }

    [[nodiscard]]
    static constexpr std::int32_t getIntegralBits() noexcept
    {
        return 32 - PRECISION;
    }

    [[nodiscard]]
    friend constexpr Fixed operator*(Fixed lhs, Fixed rhs) noexcept
    {
        const std::int64_t product =
            (static_cast<std::int64_t>(lhs.x_) * static_cast<std::int64_t>(rhs.x_)) >> PRECISION;
        assert(product == static_cast<std::int32_t>(product));
        return fromRaw(static_cast<std::int32_t>(product));
    }

    [[nodiscard]]
    friend constexpr Fixed operator/(Fixed lhs, Fixed rhs) noexcept
    {
        const std::int64_t numerator = static_cast<std::int64_t>(lhs.x_) << P;
        const std::int64_t denominator = static_cast<std::int64_t>(rhs.x_);
        assert(denominator != 0);
        const std::int64_t quotient = numerator / denominator;
        assert(quotient == static_cast<std::int32_t>(quotient));
        return fromRaw(static_cast<std::int32_t>(quotient));
    }

private:
    struct RawTag
    {
    };

    constexpr Fixed(std::int32_t raw_value, RawTag) noexcept
        : x_(raw_value)
    {
    }

    std::int32_t x_{0};

    friend class Fixed16;
    friend class FixedF;
    friend class FixedI;
};


template <int P>
    requires(P > 0 && P < 32)
constexpr float Fixed<P>::getFloat() const noexcept
{
    return detail::fixedToFloat(x_, P);
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P>::Fixed(float value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(value, P);
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P>::Fixed(double value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(static_cast<float>(value), P);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P>::Fixed(std::int32_t value) noexcept
{
    assert(static_cast<float>(value >= 0 ? value : -value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::intToFixed(value, P);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P>::Fixed(std::int32_t numerator, std::int32_t denominator) noexcept
{
    const auto abs64 = [](std::int64_t value) constexpr noexcept
    {
        return value < 0 ? -value : value;
    };
    assert(abs64(numerator) < (std::int64_t{1} << (31 - P)) * abs64(denominator));
    *this = fromRaw(numerator) / fromRaw(denominator);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr std::int32_t Fixed<P>::getInt() const noexcept
{
    return detail::fixedToInt(x_, P);
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P>& Fixed<P>::operator=(float value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(value, P);
    return *this;
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P>& Fixed<P>::operator=(double value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(static_cast<float>(value), P);
    return *this;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P>& Fixed<P>::operator=(std::int32_t value) noexcept
{
    assert(static_cast<float>(value >= 0 ? value : -value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::intToFixed(value, P);
    return *this;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr bool Fixed<P>::operator>(std::int32_t value) const noexcept
{
    return *this > Fixed{value};
}

template <int P>
    requires(P > 0 && P < 32)
constexpr bool Fixed<P>::operator<(std::int32_t value) const noexcept
{
    return *this < Fixed{value};
}

template <int P>
    requires(P > 0 && P < 32)
constexpr bool Fixed<P>::operator>=(std::int32_t value) const noexcept
{
    return *this >= Fixed{value};
}

template <int P>
    requires(P > 0 && P < 32)
constexpr bool Fixed<P>::operator<=(std::int32_t value) const noexcept
{
    return *this <= Fixed{value};
}

template <int P>
    requires(P > 0 && P < 32)
constexpr bool Fixed<P>::operator==(std::int32_t value) const noexcept
{
    return *this == Fixed{value};
}

template <int P>
    requires(P > 0 && P < 32)
constexpr bool Fixed<P>::operator!=(std::int32_t value) const noexcept
{
    return *this != Fixed{value};
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::operator+(Fixed other) const noexcept
{
    const std::int64_t sum = static_cast<std::int64_t>(x_) + static_cast<std::int64_t>(other.x_);
    assert(sum == static_cast<std::int32_t>(sum));
    return fromRaw(static_cast<std::int32_t>(sum));
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::operator-(Fixed other) const noexcept
{
    const std::int64_t difference = static_cast<std::int64_t>(x_) - static_cast<std::int64_t>(other.x_);
    assert(difference == static_cast<std::int32_t>(difference));
    return fromRaw(static_cast<std::int32_t>(difference));
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P>& Fixed<P>::operator+=(Fixed other) noexcept
{
    *this = *this + other;
    return *this;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P>& Fixed<P>::operator-=(Fixed other) noexcept
{
    *this = *this - other;
    return *this;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::operator-() const noexcept
{
    // |INT_MIN| is one greater than INT_MAX, so map INT_MIN to -INT_MAX.
    const std::int32_t clamped = x_ < -std::numeric_limits<std::int32_t>::max()
                                     ? -std::numeric_limits<std::int32_t>::max()
                                     : x_;
    return fromRaw(-clamped);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::operator*(std::int32_t value) const noexcept
{
    return fromRaw(x_ * value);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::operator/(std::int32_t value) const noexcept
{
    assert(value != 0);
    return fromRaw(x_ / value);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::abs() const noexcept
{
    return (*this < 0) ? -(*this) : *this;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::round() const noexcept
{
    constexpr std::int32_t HALF = 1 << (P - 1);
    constexpr std::int32_t MASK = static_cast<std::int32_t>(0xffffffffu << P);
    return fromRaw((x_ + HALF) & MASK);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::floor() const noexcept
{
    constexpr std::int32_t MASK = static_cast<std::int32_t>(0xffffffffu << P);
    return fromRaw(x_ & MASK);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::ceil() const noexcept
{
    constexpr std::int32_t UNIT = 1 << P;
    constexpr std::int32_t MASK = static_cast<std::int32_t>(0xffffffffu << P);
    return fromRaw((x_ & MASK) + UNIT);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P>& Fixed<P>::operator*=(Fixed other) noexcept
{
    *this = (*this) * other;
    return *this;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::square() const noexcept
{
    const Fixed product = (*this) * (*this);
    assert(product.x_ >= 0);
    return product;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::inverse() const noexcept
{
    return Fixed{1} / (*this);
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::sqrt() const noexcept
{
    assert(x_ >= 0);
    Fixed result = fromRaw((x_ + (1 << PRECISION)) >> 1);
    for (int i = 0; i < PRECISION; ++i)
    {
        result = fromRaw((result.x_ + ((*this) / result).x_) >> 1);
    }
    return result;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::invSqrt() const noexcept
{
    assert(x_ >= 0);
    return Fixed{1} / sqrt();
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::inverseApprox() const noexcept
{
    return inverse();
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::sqrtApprox() const noexcept
{
    assert(x_ >= 0);
    Fixed result = fromRaw((x_ + (1 << PRECISION)) >> 1);
    for (int i = 0; i < PRECISION / 2; ++i)
    {
        result = fromRaw((result.x_ + ((*this) / result).x_) >> 1);
    }
    return result;
}

template <int P>
    requires(P > 0 && P < 32)
constexpr Fixed<P> Fixed<P>::invSqrtApprox() const noexcept
{
    assert(x_ >= 0);
    return Fixed{1} / sqrtApprox();
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P> Fixed<P>::sin() const noexcept
{
    return fromRaw(detail::floatToFixed(std::sin(getFloat()), P));
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P> Fixed<P>::cos() const noexcept
{
    return fromRaw(detail::floatToFixed(std::cos(getFloat()), P));
}

template <int P>
    requires(P > 0 && P < 32)
inline void Fixed<P>::sinCos(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    sin_result = fromRaw(detail::floatToFixed(std::sin(getFloat()), P));
    cos_result = fromRaw(detail::floatToFixed(std::cos(getFloat()), P));
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P> Fixed<P>::sinApprox() const noexcept
{
    return sin();
}

template <int P>
    requires(P > 0 && P < 32)
inline Fixed<P> Fixed<P>::cosApprox() const noexcept
{
    return cos();
}

template <int P>
    requires(P > 0 && P < 32)
inline void Fixed<P>::sinCosApprox(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    sinCos(sin_result, cos_result);
}

template <int P>
    requires(P > 0 && P < 32)
[[nodiscard]]
constexpr Fixed<P> operator-(std::int32_t value, Fixed<P> fixed) noexcept
{
    return Fixed<P>{value} - fixed;
}

template <int P>
    requires(P > 0 && P < 32)
[[nodiscard]]
constexpr Fixed<P> operator*(std::int32_t value, Fixed<P> fixed) noexcept
{
    return fixed * value;
}
