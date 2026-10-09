#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>

class Fixed16;
class FixedF;
class FixedI;

namespace detail
{

[[nodiscard]]
inline std::int32_t floatToFixed16(float value) noexcept
{
    return static_cast<std::int32_t>(value * static_cast<float>(1 << 16));
}

[[nodiscard]]
inline std::int32_t floatToFixed(float value, int fractional_bits) noexcept
{
    return static_cast<std::int32_t>(value * static_cast<float>(1 << fractional_bits));
}

[[nodiscard]]
inline std::int32_t intToFixed16(std::int32_t value) noexcept
{
    return value << 16;
}

[[nodiscard]]
inline std::int32_t intToFixed(std::int32_t value, int fractional_bits) noexcept
{
    return value << fractional_bits;
}

[[nodiscard]]
inline float fixed16ToFloat(std::int32_t value) noexcept
{
    return static_cast<float>(value) / static_cast<float>(1 << 16);
}

[[nodiscard]]
inline float fixedToFloat(std::int32_t value, int fractional_bits) noexcept
{
    return static_cast<float>(value) / static_cast<float>(1 << fractional_bits);
}

[[nodiscard]]
inline std::int32_t fixed16ToInt(std::int32_t value) noexcept
{
    return value >> 16;
}

[[nodiscard]]
inline std::int32_t fixedToInt(std::int32_t value, int fractional_bits) noexcept
{
    return value >> fractional_bits;
}

} // namespace detail


// Fixed-point number with P fractional bits in a signed 32-bit container (Q(32-P).P).
template <int P>
class Fixed
{
public:
    static constexpr int PRECISION = P;

    Fixed() noexcept
        : x_(0)
    {
    }

    explicit Fixed(float value) noexcept;
    explicit Fixed(double value) noexcept;
    explicit Fixed(std::int32_t value) noexcept;
    Fixed(std::int32_t numerator, std::int32_t denominator) noexcept;

    Fixed& operator=(float value) noexcept;
    Fixed& operator=(double value) noexcept;
    Fixed& operator=(std::int32_t value) noexcept;

    [[nodiscard]] bool operator==(const Fixed& other) const noexcept;
    [[nodiscard]] bool operator!=(const Fixed& other) const noexcept;
    [[nodiscard]] bool operator>(const Fixed& other) const noexcept;
    [[nodiscard]] bool operator<(const Fixed& other) const noexcept;
    [[nodiscard]] bool operator>=(const Fixed& other) const noexcept;
    [[nodiscard]] bool operator<=(const Fixed& other) const noexcept;
    [[nodiscard]] bool operator>(std::int32_t value) const noexcept;
    [[nodiscard]] bool operator<(std::int32_t value) const noexcept;
    [[nodiscard]] bool operator>=(std::int32_t value) const noexcept;
    [[nodiscard]] bool operator<=(std::int32_t value) const noexcept;

    [[nodiscard]] Fixed operator+(Fixed other) const noexcept;
    [[nodiscard]] Fixed operator-(Fixed other) const noexcept;
    [[nodiscard]] Fixed operator*(std::int32_t value) const noexcept;
    [[nodiscard]] Fixed operator/(std::int32_t value) const noexcept;
    Fixed& operator+=(Fixed other) noexcept;
    Fixed& operator-=(Fixed other) noexcept;
    Fixed& operator*=(Fixed other) noexcept;
    [[nodiscard]] Fixed operator-() const noexcept;

    [[nodiscard]] Fixed abs() const noexcept;
    [[nodiscard]] Fixed square() const noexcept;
    [[nodiscard]] Fixed inverse() const noexcept;
    [[nodiscard]] Fixed sqrt() const noexcept;
    [[nodiscard]] Fixed invSqrt() const noexcept;
    [[nodiscard]] Fixed round() const noexcept;
    [[nodiscard]] Fixed floor() const noexcept;
    [[nodiscard]] Fixed ceil() const noexcept;

    [[nodiscard]] Fixed inverseApprox() const noexcept;
    [[nodiscard]] Fixed sqrtApprox() const noexcept;
    [[nodiscard]] Fixed invSqrtApprox() const noexcept;

    [[nodiscard]] Fixed sin() const noexcept;
    [[nodiscard]] Fixed cos() const noexcept;
    void sinCos(Fixed& sin_result, Fixed& cos_result) const noexcept;

    [[nodiscard]] Fixed sinApprox() const noexcept;
    [[nodiscard]] Fixed cosApprox() const noexcept;
    void sinCosApprox(Fixed& sin_result, Fixed& cos_result) const noexcept;

    [[nodiscard]] float getFloat() const noexcept;
    [[nodiscard]] std::int32_t getInt() const noexcept;

    [[nodiscard]] std::int32_t getRawValue() const noexcept
    {
        return x_;
    }

    void setRawValue(std::int32_t raw_value) noexcept
    {
        x_ = raw_value;
    }

    [[nodiscard]]
    static Fixed getMaxValue() noexcept
    {
        return Fixed(std::numeric_limits<std::int32_t>::max(), true);
    }

    [[nodiscard]]
    static Fixed getMinValue() noexcept
    {
        return Fixed(std::numeric_limits<std::int32_t>::min(), true);
    }

    [[nodiscard]]
    static Fixed getEpsilon() noexcept
    {
        return Fixed(1, true);
    }

    [[nodiscard]]
    static std::int32_t getFractionalBits() noexcept
    {
        return PRECISION;
    }

    [[nodiscard]]
    static std::int32_t getIntegralBits() noexcept
    {
        return 32 - PRECISION;
    }

    friend Fixed operator*(Fixed lhs, Fixed rhs) noexcept
    {
        Fixed result;
        const std::int64_t product =
            (static_cast<std::int64_t>(lhs.x_) * static_cast<std::int64_t>(rhs.x_)) >> PRECISION;
        result.x_ = static_cast<std::int32_t>(product);
        assert(product == static_cast<std::int64_t>(result.x_));
        return result;
    }

    friend Fixed operator/(Fixed lhs, Fixed rhs) noexcept
    {
        Fixed result;
        std::int64_t numerator = static_cast<std::int64_t>(lhs.x_);
        numerator <<= P;
        const std::int64_t denominator = static_cast<std::int64_t>(rhs.x_);
        assert(denominator != 0);
        const std::int64_t quotient = numerator / denominator;
        result.x_ = static_cast<std::int32_t>(quotient);
        assert(quotient == static_cast<std::int64_t>(result.x_));
        return result;
    }

private:
    Fixed(std::int32_t raw_value, bool /*raw_tag*/) noexcept
        : x_(raw_value)
    {
    }

    std::int32_t x_;

    friend class Fixed16;
    friend class FixedF;
    friend class FixedI;
};


template <>
inline float Fixed<16>::getFloat() const noexcept
{
    return detail::fixed16ToFloat(x_);
}

template <int P>
inline float Fixed<P>::getFloat() const noexcept
{
    return detail::fixedToFloat(x_, P);
}

template <>
inline Fixed<16>::Fixed(float value) noexcept
{
    assert(std::fabs(value) <= Fixed<16>::getMaxValue().getFloat());
    x_ = detail::floatToFixed16(value);
}

template <int P>
inline Fixed<P>::Fixed(float value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(value, P);
}

template <>
inline Fixed<16>::Fixed(double value) noexcept
{
    assert(std::fabs(value) <= Fixed<16>::getMaxValue().getFloat());
    x_ = detail::floatToFixed16(static_cast<float>(value));
}

template <int P>
inline Fixed<P>::Fixed(double value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(static_cast<float>(value), P);
}

template <>
inline Fixed<16>::Fixed(std::int32_t value) noexcept
{
    assert(static_cast<float>(std::abs(value)) <= Fixed<16>::getMaxValue().getFloat());
    x_ = detail::intToFixed16(value);
}

template <int P>
inline Fixed<P>::Fixed(std::int32_t value) noexcept
{
    assert(static_cast<float>(std::abs(value)) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::intToFixed(value, P);
}

template <int P>
inline Fixed<P>::Fixed(std::int32_t numerator, std::int32_t denominator) noexcept
{
    assert(std::llabs(static_cast<std::int64_t>(numerator)) <
           (static_cast<std::int64_t>(1) << (31 - P)) * std::llabs(static_cast<std::int64_t>(denominator)));
    *this = Fixed<P>(numerator, true) / Fixed<P>(denominator, true);
}

template <>
inline std::int32_t Fixed<16>::getInt() const noexcept
{
    return detail::fixed16ToInt(x_);
}

template <int P>
inline std::int32_t Fixed<P>::getInt() const noexcept
{
    return detail::fixedToInt(x_, P);
}

template <>
inline Fixed<16>& Fixed<16>::operator=(float value) noexcept
{
    assert(std::fabs(value) <= Fixed<16>::getMaxValue().getFloat());
    x_ = detail::floatToFixed16(value);
    return *this;
}

template <int P>
inline Fixed<P>& Fixed<P>::operator=(float value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(value, P);
    return *this;
}

template <>
inline Fixed<16>& Fixed<16>::operator=(double value) noexcept
{
    assert(std::fabs(value) <= Fixed<16>::getMaxValue().getFloat());
    x_ = detail::floatToFixed16(static_cast<float>(value));
    return *this;
}

template <int P>
inline Fixed<P>& Fixed<P>::operator=(double value) noexcept
{
    assert(std::fabs(value) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::floatToFixed(static_cast<float>(value), P);
    return *this;
}

template <int P>
inline Fixed<P>& Fixed<P>::operator=(std::int32_t value) noexcept
{
    assert(static_cast<float>(std::abs(value)) <= Fixed<P>::getMaxValue().getFloat());
    x_ = detail::intToFixed(value, P);
    return *this;
}

template <int P>
inline bool Fixed<P>::operator==(const Fixed& other) const noexcept
{
    return x_ == other.x_;
}

template <int P>
inline bool Fixed<P>::operator!=(const Fixed& other) const noexcept
{
    return x_ != other.x_;
}

template <int P>
inline bool Fixed<P>::operator>(const Fixed& other) const noexcept
{
    return x_ > other.x_;
}

template <int P>
inline bool Fixed<P>::operator<(const Fixed& other) const noexcept
{
    return x_ < other.x_;
}

template <int P>
inline bool Fixed<P>::operator>=(const Fixed& other) const noexcept
{
    return x_ >= other.x_;
}

template <int P>
inline bool Fixed<P>::operator<=(const Fixed& other) const noexcept
{
    return x_ <= other.x_;
}

template <int P>
inline bool Fixed<P>::operator>(std::int32_t value) const noexcept
{
    return *this > Fixed<P>(value);
}

template <int P>
inline bool Fixed<P>::operator<(std::int32_t value) const noexcept
{
    return *this < Fixed<P>(value);
}

template <int P>
inline bool Fixed<P>::operator>=(std::int32_t value) const noexcept
{
    return *this >= Fixed<P>(value);
}

template <int P>
inline bool Fixed<P>::operator<=(std::int32_t value) const noexcept
{
    return *this <= Fixed<P>(value);
}

template <int P>
inline Fixed<P> Fixed<P>::operator+(Fixed other) const noexcept
{
    Fixed result;
    result.x_ = x_ + other.x_;
    assert(static_cast<std::int64_t>(result.x_) ==
           static_cast<std::int64_t>(x_) + static_cast<std::int64_t>(other.x_));
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::operator-(Fixed other) const noexcept
{
    Fixed result;
    result.x_ = x_ - other.x_;
    assert(static_cast<std::int64_t>(result.x_) ==
           static_cast<std::int64_t>(x_) - static_cast<std::int64_t>(other.x_));
    return result;
}

template <int P>
inline Fixed<P>& Fixed<P>::operator+=(Fixed other) noexcept
{
    assert(static_cast<std::int64_t>(x_) + static_cast<std::int64_t>(other.x_) ==
           static_cast<std::int64_t>(x_ + other.x_));
    x_ += other.x_;
    return *this;
}

template <int P>
inline Fixed<P>& Fixed<P>::operator-=(Fixed other) noexcept
{
    assert(static_cast<std::int64_t>(x_) - static_cast<std::int64_t>(other.x_) ==
           static_cast<std::int64_t>(x_ - other.x_));
    x_ -= other.x_;
    return *this;
}

template <int P>
inline Fixed<P> Fixed<P>::operator-() const noexcept
{
    Fixed result;
    // |INT_MIN| is one greater than INT_MAX, so map INT_MIN to -INT_MAX.
    result.x_ = -std::max(-std::numeric_limits<std::int32_t>::max(), x_);
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::operator*(std::int32_t value) const noexcept
{
    Fixed result;
    result.x_ = x_ * value;
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::operator/(std::int32_t value) const noexcept
{
    assert(value != 0);
    Fixed result;
    result.x_ = x_ / value;
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::abs() const noexcept
{
    Fixed result(*this);
    if (result < 0)
    {
        result = -result;
    }
    return result;
}

template <>
inline Fixed<16> Fixed<16>::round() const noexcept
{
    Fixed<16> result;
    result.x_ = (x_ + 0x8000) & static_cast<std::int32_t>(0xffff0000u);
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::round() const noexcept
{
    Fixed result;
    result.x_ = (x_ + (1 << (P - 1))) & static_cast<std::int32_t>(0xffffffffu << P);
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::floor() const noexcept
{
    Fixed result;
    result.x_ = x_ & static_cast<std::int32_t>(0xffffffffu << P);
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::ceil() const noexcept
{
    Fixed result;
    result.x_ = (x_ & static_cast<std::int32_t>(0xffffffffu << P)) + (1 << P);
    return result;
}

template <int P>
inline Fixed<P>& Fixed<P>::operator*=(Fixed other) noexcept
{
    *this = (*this) * other;
    return *this;
}

template <int P>
inline Fixed<P> Fixed<P>::square() const noexcept
{
    const Fixed product = (*this) * (*this);
    assert(product.x_ >= 0);
    return product;
}

template <int P>
inline Fixed<P> Fixed<P>::inverse() const noexcept
{
    return Fixed(1) / (*this);
}

template <int P>
inline Fixed<P> Fixed<P>::sqrt() const noexcept
{
    assert(x_ >= 0);
    Fixed result;
    result.x_ = (x_ + (1 << PRECISION)) >> 1;
    for (int i = 0; i < PRECISION; ++i)
    {
        result.x_ = (result.x_ + ((*this) / result).x_) >> 1;
    }
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::invSqrt() const noexcept
{
    assert(x_ >= 0);
    return Fixed(1) / sqrt();
}

template <int P>
inline Fixed<P> Fixed<P>::inverseApprox() const noexcept
{
    return inverse();
}

template <int P>
inline Fixed<P> Fixed<P>::sqrtApprox() const noexcept
{
    assert(x_ >= 0);
    Fixed result;
    result.x_ = (x_ + (1 << PRECISION)) >> 1;
    for (int i = 0; i < PRECISION / 2; ++i)
    {
        result.x_ = (result.x_ + ((*this) / result).x_) >> 1;
    }
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::invSqrtApprox() const noexcept
{
    assert(x_ >= 0);
    return Fixed(1) / sqrtApprox();
}

template <>
inline Fixed<16> Fixed<16>::sin() const noexcept
{
    Fixed<16> result;
    result.x_ = detail::floatToFixed16(std::sin(getFloat()));
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::sin() const noexcept
{
    Fixed result;
    result.x_ = detail::floatToFixed(std::sin(getFloat()), P);
    return result;
}

template <>
inline Fixed<16> Fixed<16>::cos() const noexcept
{
    Fixed<16> result;
    result.x_ = detail::floatToFixed16(std::cos(getFloat()));
    return result;
}

template <int P>
inline Fixed<P> Fixed<P>::cos() const noexcept
{
    Fixed result;
    result.x_ = detail::floatToFixed(std::cos(getFloat()), P);
    return result;
}

template <>
inline void Fixed<16>::sinCos(Fixed<16>& sin_result, Fixed<16>& cos_result) const noexcept
{
    sin_result.x_ = detail::floatToFixed16(std::sin(getFloat()));
    cos_result.x_ = detail::floatToFixed16(std::cos(getFloat()));
}

template <int P>
inline void Fixed<P>::sinCos(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    sin_result.x_ = detail::floatToFixed(std::sin(getFloat()), P);
    cos_result.x_ = detail::floatToFixed(std::cos(getFloat()), P);
}

template <int P>
inline Fixed<P> Fixed<P>::sinApprox() const noexcept
{
    return sin();
}

template <int P>
inline Fixed<P> Fixed<P>::cosApprox() const noexcept
{
    return cos();
}

template <int P>
inline void Fixed<P>::sinCosApprox(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    sinCos(sin_result, cos_result);
}

template <int P>
[[nodiscard]]
inline Fixed<P> operator-(std::int32_t value, Fixed<P> fixed) noexcept
{
    return Fixed<P>(value) - fixed;
}

template <int P>
[[nodiscard]]
inline Fixed<P> operator*(std::int32_t value, Fixed<P> fixed) noexcept
{
    return fixed * value;
}
