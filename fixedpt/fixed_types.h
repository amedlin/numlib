#pragma once

#include "fixedpt/fixed.h"

// Q16.16 fixed-point number.
//
// Epsilon = 1/2^16
// Max ≈ 32768 - epsilon, Min = -32768
class Fixed16 : public Fixed<16>
{
public:
    using Fixed<16>::Fixed;
    using Fixed<16>::operator=;

    constexpr Fixed16() noexcept = default;

    constexpr Fixed16(Fixed<16> value) noexcept
        : Fixed<16>(value)
    {
    }

    explicit Fixed16(FixedF value) noexcept;

    Fixed16& operator=(const FixedF& value) noexcept;

    [[nodiscard]]
    constexpr Fixed16 operator*(std::int32_t value) const noexcept
    {
        return Fixed16{Fixed<16>::operator*(value)};
    }

    [[nodiscard]] Fixed16 operator*(FixedF value) const noexcept;
    [[nodiscard]] FixedI operator*(FixedI value) const noexcept;

    [[nodiscard]]
    constexpr Fixed16 operator/(std::int32_t value) const noexcept
    {
        return Fixed16{Fixed<16>::operator/(value)};
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
    using Fixed<24>::Fixed;
    using Fixed<24>::operator=;

    constexpr FixedF() noexcept = default;

    constexpr FixedF(Fixed<24> value) noexcept
        : Fixed<24>(value)
    {
    }

    explicit FixedF(Fixed16 value) noexcept
    {
        *this = value;
    }

    FixedF& operator=(const Fixed<16>& value) noexcept;

    [[nodiscard]]
    constexpr FixedF operator*(std::int32_t value) const noexcept
    {
        return FixedF{Fixed<24>::operator*(value)};
    }

    [[nodiscard]]
    constexpr FixedF operator/(std::int32_t value) const noexcept
    {
        return FixedF{Fixed<24>::operator/(value)};
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
    using Fixed<8>::Fixed;
    using Fixed<8>::operator=;

    constexpr FixedI() noexcept = default;

    constexpr FixedI(Fixed<8> value) noexcept
        : Fixed<8>(value)
    {
    }

    explicit FixedI(Fixed16 value) noexcept
    {
        *this = value;
    }

    FixedI& operator=(const Fixed<16>& value) noexcept;

    [[nodiscard]]
    constexpr FixedI operator/(std::int32_t value) const noexcept
    {
        return FixedI{Fixed<8>::operator/(value)};
    }

    [[nodiscard]]
    constexpr FixedI operator*(std::int32_t value) const noexcept
    {
        return FixedI{Fixed<8>::operator*(value)};
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
    // Same bit-width multiply as Q16.16 × Q16.16 on the raw representations.
    return Fixed16{Fixed<16>::fromRaw(x_) * Fixed<16>::fromRaw(value.x_)};
}

inline Fixed16 FixedF::operator*(Fixed16 value) const noexcept
{
    const std::int64_t product =
        (static_cast<std::int64_t>(x_) * static_cast<std::int64_t>(value.x_)) >> PRECISION;
    assert(product == static_cast<std::int32_t>(product));
    return Fixed16{Fixed<16>::fromRaw(static_cast<std::int32_t>(product))};
}

inline FixedF FixedF::operator/(FixedI value) const noexcept
{
    assert(value.x_ != 0);
    const std::int64_t quotient = (static_cast<std::int64_t>(x_) << 8) / static_cast<std::int64_t>(value.x_);
    assert(quotient == static_cast<std::int32_t>(quotient));
    return FixedF{fromRaw(static_cast<std::int32_t>(quotient))};
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
    const std::int64_t product = (static_cast<std::int64_t>(x_) * static_cast<std::int64_t>(value.x_)) >> 16;
    assert(product == static_cast<std::int32_t>(product));
    return FixedI{fromRaw(static_cast<std::int32_t>(product))};
}

inline FixedI FixedI::operator/(Fixed16 value) const noexcept
{
    const std::int64_t quotient = (static_cast<std::int64_t>(x_) << 16) / static_cast<std::int64_t>(value.x_);
    assert(quotient == static_cast<std::int32_t>(quotient));
    return FixedI{fromRaw(static_cast<std::int32_t>(quotient))};
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
