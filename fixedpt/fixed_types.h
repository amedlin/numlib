#pragma once

#include "fixedpt/fixed.h"

// Q16.16 fixed-point number.
//
// Epsilon = 1/2^16
// Max ≈ 32768 - epsilon, Min = -32768
class Fixed16 : public Fixed<16>
{
public:
    Fixed16() noexcept = default;

    Fixed16(Fixed<16> value) noexcept
        : Fixed<16>(value)
    {
    }

    explicit Fixed16(float value) noexcept
        : Fixed<16>(value)
    {
    }

    explicit Fixed16(double value) noexcept
        : Fixed<16>(value)
    {
    }

    explicit Fixed16(std::int32_t value) noexcept
        : Fixed<16>(value)
    {
    }

    Fixed16(std::int32_t numerator, std::int32_t denominator) noexcept
        : Fixed<16>(numerator, denominator)
    {
    }

    explicit Fixed16(FixedF value) noexcept;

    Fixed16& operator=(const FixedF& value) noexcept;

    Fixed16& operator=(std::int32_t value) noexcept
    {
        *static_cast<Fixed<16>*>(this) = value;
        return *this;
    }

    Fixed16& operator=(float value) noexcept
    {
        *static_cast<Fixed<16>*>(this) = value;
        return *this;
    }

    Fixed16& operator=(double value) noexcept
    {
        *static_cast<Fixed<16>*>(this) = value;
        return *this;
    }

    [[nodiscard]]
    Fixed16 operator*(std::int32_t value) const noexcept
    {
        return (*static_cast<const Fixed<16>*>(this)) * value;
    }

    [[nodiscard]] Fixed16 operator*(FixedF value) const noexcept;
    [[nodiscard]] FixedI operator*(FixedI value) const noexcept;

    [[nodiscard]]
    Fixed16 operator/(std::int32_t value) const noexcept
    {
        return Fixed<16>::operator/(value);
    }

    [[nodiscard]] Fixed16 expApprox() const noexcept;
};


// Q8.24 fixed-point number.
//
// Epsilon = 1/2^24
// Max ≈ 128 - epsilon, Min = -128
class FixedF : public Fixed<24>
{
public:
    FixedF() noexcept = default;

    FixedF(Fixed<24> value) noexcept
        : Fixed<24>(value)
    {
    }

    explicit FixedF(float value) noexcept
        : Fixed<24>(value)
    {
    }

    explicit FixedF(double value) noexcept
        : Fixed<24>(value)
    {
    }

    explicit FixedF(std::int32_t value) noexcept
        : Fixed<24>(value)
    {
    }

    FixedF(std::int32_t numerator, std::int32_t denominator) noexcept
        : Fixed<24>(numerator, denominator)
    {
    }

    explicit FixedF(Fixed16 value) noexcept
    {
        *this = value;
    }

    FixedF& operator=(const Fixed<16>& value) noexcept;

    FixedF& operator=(std::int32_t value) noexcept
    {
        *static_cast<Fixed<24>*>(this) = value;
        return *this;
    }

    FixedF& operator=(float value) noexcept
    {
        *static_cast<Fixed<24>*>(this) = value;
        return *this;
    }

    FixedF& operator=(double value) noexcept
    {
        *static_cast<Fixed<24>*>(this) = value;
        return *this;
    }

    [[nodiscard]]
    FixedF operator*(std::int32_t value) const noexcept
    {
        return static_cast<Fixed<24>>(*this) * value;
    }

    [[nodiscard]]
    FixedF operator/(std::int32_t value) const noexcept
    {
        return static_cast<Fixed<24>>(*this) / value;
    }

    [[nodiscard]] Fixed16 operator*(FixedI value) const noexcept;
    [[nodiscard]] Fixed16 operator*(Fixed16 value) const noexcept;
    [[nodiscard]] FixedF operator/(FixedI value) const noexcept;
};


// Q24.8 fixed-point number.
//
// Epsilon = 1/2^8
// Max ≈ 8388608 - epsilon, Min = -8388608
class FixedI : public Fixed<8>
{
public:
    FixedI() noexcept = default;

    FixedI(Fixed<8> value) noexcept
        : Fixed<8>(value)
    {
    }

    explicit FixedI(float value) noexcept
        : Fixed<8>(value)
    {
    }

    explicit FixedI(double value) noexcept
        : Fixed<8>(value)
    {
    }

    explicit FixedI(std::int32_t value) noexcept
        : Fixed<8>(value)
    {
    }

    FixedI(std::int32_t numerator, std::int32_t denominator) noexcept
        : Fixed<8>(numerator, denominator)
    {
    }

    explicit FixedI(Fixed16 value) noexcept
    {
        *this = value;
    }

    FixedI& operator=(const Fixed<16>& value) noexcept;

    FixedI& operator=(std::int32_t value) noexcept
    {
        *static_cast<Fixed<8>*>(this) = value;
        return *this;
    }

    FixedI& operator=(float value) noexcept
    {
        *static_cast<Fixed<8>*>(this) = value;
        return *this;
    }

    FixedI& operator=(double value) noexcept
    {
        *static_cast<Fixed<8>*>(this) = value;
        return *this;
    }

    [[nodiscard]]
    FixedI operator/(std::int32_t value) const noexcept
    {
        return static_cast<Fixed<8>>(*this) / value;
    }

    [[nodiscard]]
    FixedI operator*(std::int32_t value) const noexcept
    {
        return static_cast<Fixed<8>>(*this) * value;
    }

    [[nodiscard]] Fixed16 operator*(FixedF value) const noexcept;
    [[nodiscard]] FixedI operator*(Fixed16 value) const noexcept;
    [[nodiscard]] FixedI operator/(Fixed16 value) const noexcept;
};


inline FixedF& FixedF::operator=(const Fixed<16>& value) noexcept
{
    assert(std::fabs(value.getFloat()) <= FixedF::getMaxValue().getFloat());
    x_ = value.x_ << 8;
    return *this;
}

inline Fixed16 FixedF::operator*(FixedI value) const noexcept
{
    return (*reinterpret_cast<const Fixed<16>*>(this)) * (*reinterpret_cast<Fixed<16>*>(&value));
}

inline Fixed16 FixedF::operator*(Fixed16 value) const noexcept
{
    Fixed16 result;
    const std::int64_t product =
        (static_cast<std::int64_t>(x_) * static_cast<std::int64_t>(value.x_)) >> PRECISION;
    result.x_ = static_cast<std::int32_t>(product);
    assert(product == static_cast<std::int64_t>(result.x_));
    return result;
}

inline FixedF FixedF::operator/(FixedI value) const noexcept
{
    assert(value.x_ != 0);
    FixedF result;
    const std::int64_t quotient = (static_cast<std::int64_t>(x_) << 8) / static_cast<std::int64_t>(value.x_);
    result.x_ = static_cast<std::int32_t>(quotient);
    assert(quotient == static_cast<std::int64_t>(result.x_));
    return result;
}

inline FixedI& FixedI::operator=(const Fixed<16>& value) noexcept
{
    x_ = value.x_ >> 8;
    return *this;
}

inline Fixed16 FixedI::operator*(FixedF value) const noexcept
{
    return value * (*this);
}

inline FixedI FixedI::operator*(Fixed16 value) const noexcept
{
    FixedI result;
    const std::int64_t product = (static_cast<std::int64_t>(x_) * static_cast<std::int64_t>(value.x_)) >> 16;
    result.x_ = static_cast<std::int32_t>(product);
    assert(product == static_cast<std::int64_t>(result.x_));
    return result;
}

inline FixedI FixedI::operator/(Fixed16 value) const noexcept
{
    FixedI result;
    const std::int64_t quotient = (static_cast<std::int64_t>(x_) << 16) / static_cast<std::int64_t>(value.x_);
    result.x_ = static_cast<std::int32_t>(quotient);
    assert(quotient == static_cast<std::int64_t>(result.x_));
    return result;
}

inline Fixed16 Fixed16::operator*(FixedF value) const noexcept
{
    return value * (*this);
}

inline FixedI Fixed16::operator*(FixedI value) const noexcept
{
    return value * (*this);
}

inline Fixed16& Fixed16::operator=(const FixedF& value) noexcept
{
    x_ = value.x_ >> 8;
    return *this;
}

inline Fixed16::Fixed16(FixedF value) noexcept
{
    *this = value;
}
