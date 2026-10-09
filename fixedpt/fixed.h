#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <compare>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace detail
{

template <class Rep>
struct Widening;

template <>
struct Widening<std::int16_t>
{
    using type = std::int32_t;
};

template <>
struct Widening<std::int32_t>
{
    using type = std::int64_t;
};

#if defined(__SIZEOF_INT128__)
template <>
struct Widening<std::int64_t>
{
    using type = __int128;
};
#endif

template <class Rep>
using WideningT = typename Widening<Rep>::type;

template <class Rep>
concept FixedRep = std::is_integral_v<Rep> && std::is_signed_v<Rep> && requires { typename Widening<Rep>::type; };

template <class Rep>
    requires FixedRep<Rep>
[[nodiscard]]
constexpr Rep floatToFixed(float value, int fractional_bits) noexcept
{
    using Unsigned = std::make_unsigned_t<Rep>;
    return static_cast<Rep>(value * static_cast<float>(Unsigned{1} << fractional_bits));
}

template <class Rep>
    requires FixedRep<Rep>
[[nodiscard]]
constexpr Rep intToFixed(Rep value, int fractional_bits) noexcept
{
    return static_cast<Rep>(value << fractional_bits);
}

template <class Rep>
    requires FixedRep<Rep>
[[nodiscard]]
constexpr float fixedToFloat(Rep value, int fractional_bits) noexcept
{
    using Unsigned = std::make_unsigned_t<Rep>;
    return static_cast<float>(value) / static_cast<float>(Unsigned{1} << fractional_bits);
}

template <class Rep>
    requires FixedRep<Rep>
[[nodiscard]]
constexpr Rep fixedToInt(Rep value, int fractional_bits) noexcept
{
    return static_cast<Rep>(value >> fractional_bits);
}

[[nodiscard]]
constexpr double constexprAtan(double x) noexcept
{
    // Taylor series valid for |x| <= 1.
    const double x2 = x * x;
    double term = x;
    double sum = 0.0;
    for (int n = 0; n < 40; ++n)
    {
        sum += term / static_cast<double>(2 * n + 1);
        term *= -x2;
    }
    return sum;
}

template <int P, int N>
[[nodiscard]]
constexpr std::array<std::int64_t, N> makeCordicAtanTable() noexcept
{
    std::array<std::int64_t, N> table{};
    constexpr double SCALE = static_cast<double>(std::int64_t{1} << P);
    constexpr double PI_OVER_4 = 0.78539816339744830961566084581988;

    table[0] = static_cast<std::int64_t>(PI_OVER_4 * SCALE + 0.5);
    for (int i = 1; i < N; ++i)
    {
        double x = 1.0;
        for (int k = 0; k < i; ++k)
        {
            x *= 0.5;
        }
        table[static_cast<std::size_t>(i)] = static_cast<std::int64_t>(constexprAtan(x) * SCALE + 0.5);
    }
    return table;
}

[[nodiscard]]
constexpr double constexprFmod(double x, double y) noexcept
{
    const double q = x / y;
    const auto n = static_cast<long long>(q);
    return x - y * static_cast<double>(n);
}

// Circular CORDIC: angle, sin, and cos share the same Q format (P fractional bits).
// Range reduction uses double so large angles stay accurate; the rotation loop is fixed-point.
template <int P, class Rep, int Iterations>
    requires FixedRep<Rep>
constexpr void cordicSinCos(Rep angle_raw, Rep& sin_raw, Rep& cos_raw) noexcept
{
    using Wide = WideningT<Rep>;
    using Unsigned = std::make_unsigned_t<Rep>;

    static constexpr int TABLE_SIZE = std::numeric_limits<Rep>::digits;
    static constexpr auto ATAN = makeCordicAtanTable<P, TABLE_SIZE>();
    static constexpr double CORDIC_K = 0.6072529350088812561694;
    static constexpr double SCALE = static_cast<double>(Unsigned{1} << P);
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double TWO_PI = 2.0 * PI;
    static constexpr double HALF_PI = 0.5 * PI;
    static constexpr Wide K = static_cast<Wide>(CORDIC_K * SCALE + 0.5);

    double z = fixedToFloat(angle_raw, P);
    z = constexprFmod(z, TWO_PI);
    if (z < 0.0)
    {
        z += TWO_PI;
    }
    if (z > PI)
    {
        z -= TWO_PI;
    }

    Wide sign_sin = 1;
    Wide sign_cos = 1;
    if (z < 0.0)
    {
        z = -z;
        sign_sin = -1;
    }
    if (z > HALF_PI)
    {
        z = PI - z;
        sign_cos = -1;
    }

    Wide z_fixed = static_cast<Wide>(z * SCALE + 0.5);
    Wide x = K;
    Wide y = 0;

    for (int i = 0; i < Iterations && i < TABLE_SIZE; ++i)
    {
        const Wide x_shift = x >> i;
        const Wide y_shift = y >> i;
        const Wide atan = static_cast<Wide>(ATAN[static_cast<std::size_t>(i)]);
        Wide next_x;
        Wide next_y;
        if (z_fixed >= 0)
        {
            next_x = x - y_shift;
            next_y = y + x_shift;
            z_fixed = z_fixed - atan;
        }
        else
        {
            next_x = x + y_shift;
            next_y = y - x_shift;
            z_fixed = z_fixed + atan;
        }
        x = next_x;
        y = next_y;
    }

    cos_raw = static_cast<Rep>(sign_cos * x);
    sin_raw = static_cast<Rep>(sign_sin * y);
}

} // namespace detail


/// Binary fixed-point number with P fractional bits in a signed Rep container.
///
/// The default representation is std::int32_t, which gives Q(32-P).P.
/// Wider signed representations are supported when a widening integer type is
/// available. Arithmetic is not saturating; debug assertions detect many
/// out-of-range results and invalid inputs.
template <int P, detail::FixedRep Rep = std::int32_t>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
class Fixed
{
public:
    static constexpr int PRECISION = P;
    using rep = Rep;

    constexpr Fixed() noexcept = default;

    explicit Fixed(float value) noexcept;
    explicit Fixed(double value) noexcept;
    explicit constexpr Fixed(Rep value) noexcept;
    constexpr Fixed(Rep numerator, Rep denominator) noexcept;

    Fixed& operator=(float value) noexcept;
    Fixed& operator=(double value) noexcept;
    constexpr Fixed& operator=(Rep value) noexcept;

    [[nodiscard]]
    constexpr bool operator==(const Fixed& other) const noexcept = default;

    [[nodiscard]]
    constexpr std::strong_ordering operator<=>(const Fixed& other) const noexcept = default;

    [[nodiscard]] constexpr bool operator>(Rep value) const noexcept;
    [[nodiscard]] constexpr bool operator<(Rep value) const noexcept;
    [[nodiscard]] constexpr bool operator>=(Rep value) const noexcept;
    [[nodiscard]] constexpr bool operator<=(Rep value) const noexcept;
    [[nodiscard]] constexpr bool operator==(Rep value) const noexcept;
    [[nodiscard]] constexpr bool operator!=(Rep value) const noexcept;

    [[nodiscard]] constexpr Fixed operator+(Fixed other) const noexcept;
    [[nodiscard]] constexpr Fixed operator-(Fixed other) const noexcept;
    [[nodiscard]] constexpr Fixed operator*(Rep value) const noexcept;
    [[nodiscard]] constexpr Fixed operator/(Rep value) const noexcept;
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

    // Fixed-domain CORDIC trig (no float).
    [[nodiscard]] constexpr Fixed sin() const noexcept;
    [[nodiscard]] constexpr Fixed cos() const noexcept;
    constexpr void sinCos(Fixed& sin_result, Fixed& cos_result) const noexcept;

    // Fewer CORDIC iterations than sin/cos.
    [[nodiscard]] constexpr Fixed sinApprox() const noexcept;
    [[nodiscard]] constexpr Fixed cosApprox() const noexcept;
    constexpr void sinCosApprox(Fixed& sin_result, Fixed& cos_result) const noexcept;

    // Float math library path (legacy portable behavior).
    [[nodiscard]] Fixed sinViaFloat() const noexcept;
    [[nodiscard]] Fixed cosViaFloat() const noexcept;
    void sinCosViaFloat(Fixed& sin_result, Fixed& cos_result) const noexcept;

    [[nodiscard]] constexpr Fixed scaleByPowerOfTwo(int exp) const noexcept;

    [[nodiscard]] constexpr float getFloat() const noexcept;
    [[nodiscard]] constexpr Rep getInt() const noexcept;

    [[nodiscard]]
    constexpr Rep getRawValue() const noexcept
    {
        return x_;
    }

    constexpr void setRawValue(Rep raw_value) noexcept
    {
        x_ = raw_value;
    }

    [[nodiscard]]
    static constexpr Fixed fromRaw(Rep raw_value) noexcept
    {
        return Fixed(raw_value, RawTag{});
    }

    [[nodiscard]]
    static constexpr Fixed getMaxValue() noexcept
    {
        return fromRaw(std::numeric_limits<Rep>::max());
    }

    [[nodiscard]]
    static constexpr Fixed getMinValue() noexcept
    {
        return fromRaw(std::numeric_limits<Rep>::min());
    }

    [[nodiscard]]
    static constexpr Fixed getEpsilon() noexcept
    {
        return fromRaw(Rep{1});
    }

    [[nodiscard]]
    static constexpr int getFractionalBits() noexcept
    {
        return PRECISION;
    }

    [[nodiscard]]
    static constexpr int getIntegralBits() noexcept
    {
        return static_cast<int>(sizeof(Rep) * 8) - PRECISION;
    }

    [[nodiscard]]
    friend constexpr Fixed operator*(Fixed lhs, Fixed rhs) noexcept
    {
        using Wide = detail::WideningT<Rep>;
        const Wide product = (static_cast<Wide>(lhs.x_) * static_cast<Wide>(rhs.x_)) >> PRECISION;
        assert(product == static_cast<Rep>(product));
        return fromRaw(static_cast<Rep>(product));
    }

    [[nodiscard]]
    friend constexpr Fixed operator/(Fixed lhs, Fixed rhs) noexcept
    {
        using Wide = detail::WideningT<Rep>;
        const Wide numerator = static_cast<Wide>(lhs.x_) << P;
        const Wide denominator = static_cast<Wide>(rhs.x_);
        assert(denominator != 0);
        const Wide quotient = numerator / denominator;
        assert(quotient == static_cast<Rep>(quotient));
        return fromRaw(static_cast<Rep>(quotient));
    }

private:
    struct RawTag
    {
    };

    constexpr Fixed(Rep raw_value, RawTag) noexcept
        : x_(raw_value)
    {
    }

    static constexpr int CORDIC_ITERS = std::numeric_limits<Rep>::digits;
    static constexpr int CORDIC_APPROX_ITERS = CORDIC_ITERS / 2;

    Rep x_{0};
};


template <int To, int From, detail::FixedRep Rep = std::int32_t>
    requires(To > 0 && To < static_cast<int>(sizeof(Rep) * 8) && From > 0 &&
             From < static_cast<int>(sizeof(Rep) * 8))
[[nodiscard]]
constexpr Fixed<To, Rep> convert(Fixed<From, Rep> value) noexcept
{
    using Wide = detail::WideningT<Rep>;
    if constexpr (To == From)
    {
        return Fixed<To, Rep>::fromRaw(value.getRawValue());
    }
    else if constexpr (To > From)
    {
        constexpr int SHIFT = To - From;
        const Wide scaled = static_cast<Wide>(value.getRawValue()) << SHIFT;
        assert(scaled == static_cast<Rep>(scaled));
        return Fixed<To, Rep>::fromRaw(static_cast<Rep>(scaled));
    }
    else
    {
        constexpr int SHIFT = From - To;
        return Fixed<To, Rep>::fromRaw(static_cast<Rep>(value.getRawValue() >> SHIFT));
    }
}


template <int Out, int P, int Q, detail::FixedRep Rep = std::int32_t>
    requires(Out > 0 && Out < static_cast<int>(sizeof(Rep) * 8) && P > 0 &&
             P < static_cast<int>(sizeof(Rep) * 8) && Q > 0 && Q < static_cast<int>(sizeof(Rep) * 8))
[[nodiscard]]
constexpr Fixed<Out, Rep> mulAs(Fixed<P, Rep> lhs, Fixed<Q, Rep> rhs) noexcept
{
    using Wide = detail::WideningT<Rep>;
    constexpr int SHIFT = P + Q - Out;
    static_assert(SHIFT >= 0);

    const Wide product = static_cast<Wide>(lhs.getRawValue()) * static_cast<Wide>(rhs.getRawValue());
    const Wide scaled = product >> SHIFT;
    assert(scaled == static_cast<Rep>(scaled));
    return Fixed<Out, Rep>::fromRaw(static_cast<Rep>(scaled));
}


template <int Out, int P, int Q, detail::FixedRep Rep = std::int32_t>
    requires(Out > 0 && Out < static_cast<int>(sizeof(Rep) * 8) && P > 0 &&
             P < static_cast<int>(sizeof(Rep) * 8) && Q > 0 && Q < static_cast<int>(sizeof(Rep) * 8))
[[nodiscard]]
constexpr Fixed<Out, Rep> divAs(Fixed<P, Rep> lhs, Fixed<Q, Rep> rhs) noexcept
{
    using Wide = detail::WideningT<Rep>;
    constexpr int SHIFT = Q - P + Out;
    static_assert(SHIFT >= 0);

    const Wide denominator = static_cast<Wide>(rhs.getRawValue());
    assert(denominator != 0);
    const Wide numerator = static_cast<Wide>(lhs.getRawValue()) << SHIFT;
    const Wide quotient = numerator / denominator;
    assert(quotient == static_cast<Rep>(quotient));
    return Fixed<Out, Rep>::fromRaw(static_cast<Rep>(quotient));
}


template <int P, detail::FixedRep Rep = std::int32_t>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
[[nodiscard]]
constexpr Fixed<P, Rep> mulAdd(Fixed<P, Rep> a, Fixed<P, Rep> b, Fixed<P, Rep> c) noexcept
{
    using Wide = detail::WideningT<Rep>;
    const Wide product =
        (static_cast<Wide>(a.getRawValue()) * static_cast<Wide>(b.getRawValue())) >> P;
    const Wide sum = product + static_cast<Wide>(c.getRawValue());
    assert(sum == static_cast<Rep>(sum));
    return Fixed<P, Rep>::fromRaw(static_cast<Rep>(sum));
}


template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr float Fixed<P, Rep>::getFloat() const noexcept
{
    return detail::fixedToFloat(x_, P);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline Fixed<P, Rep>::Fixed(float value) noexcept
{
    assert(std::fabs(value) <= Fixed::getMaxValue().getFloat());
    x_ = detail::floatToFixed<Rep>(value, P);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline Fixed<P, Rep>::Fixed(double value) noexcept
{
    assert(std::fabs(value) <= Fixed::getMaxValue().getFloat());
    x_ = detail::floatToFixed<Rep>(static_cast<float>(value), P);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep>::Fixed(Rep value) noexcept
{
    using Wide = detail::WideningT<Rep>;
    const Wide abs_value = value < 0 ? -static_cast<Wide>(value) : static_cast<Wide>(value);
    assert(static_cast<float>(abs_value) <= Fixed::getMaxValue().getFloat());
    x_ = detail::intToFixed(value, P);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep>::Fixed(Rep numerator, Rep denominator) noexcept
{
    using Wide = detail::WideningT<Rep>;
    const auto abs_wide = [](Wide value) constexpr noexcept
    {
        return value < 0 ? -value : value;
    };
    constexpr int BITS = static_cast<int>(sizeof(Rep) * 8);
    assert(abs_wide(numerator) < (Wide{1} << (BITS - 1 - P)) * abs_wide(denominator));
    *this = fromRaw(numerator) / fromRaw(denominator);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Rep Fixed<P, Rep>::getInt() const noexcept
{
    return detail::fixedToInt(x_, P);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline Fixed<P, Rep>& Fixed<P, Rep>::operator=(float value) noexcept
{
    assert(std::fabs(value) <= Fixed::getMaxValue().getFloat());
    x_ = detail::floatToFixed<Rep>(value, P);
    return *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline Fixed<P, Rep>& Fixed<P, Rep>::operator=(double value) noexcept
{
    assert(std::fabs(value) <= Fixed::getMaxValue().getFloat());
    x_ = detail::floatToFixed<Rep>(static_cast<float>(value), P);
    return *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep>& Fixed<P, Rep>::operator=(Rep value) noexcept
{
    using Wide = detail::WideningT<Rep>;
    const Wide abs_value = value < 0 ? -static_cast<Wide>(value) : static_cast<Wide>(value);
    assert(static_cast<float>(abs_value) <= Fixed::getMaxValue().getFloat());
    x_ = detail::intToFixed(value, P);
    return *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr bool Fixed<P, Rep>::operator>(Rep value) const noexcept
{
    return *this > Fixed{value};
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr bool Fixed<P, Rep>::operator<(Rep value) const noexcept
{
    return *this < Fixed{value};
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr bool Fixed<P, Rep>::operator>=(Rep value) const noexcept
{
    return *this >= Fixed{value};
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr bool Fixed<P, Rep>::operator<=(Rep value) const noexcept
{
    return *this <= Fixed{value};
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr bool Fixed<P, Rep>::operator==(Rep value) const noexcept
{
    return *this == Fixed{value};
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr bool Fixed<P, Rep>::operator!=(Rep value) const noexcept
{
    return *this != Fixed{value};
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::operator+(Fixed other) const noexcept
{
    using Wide = detail::WideningT<Rep>;
    const Wide sum = static_cast<Wide>(x_) + static_cast<Wide>(other.x_);
    assert(sum == static_cast<Rep>(sum));
    return fromRaw(static_cast<Rep>(sum));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::operator-(Fixed other) const noexcept
{
    using Wide = detail::WideningT<Rep>;
    const Wide difference = static_cast<Wide>(x_) - static_cast<Wide>(other.x_);
    assert(difference == static_cast<Rep>(difference));
    return fromRaw(static_cast<Rep>(difference));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep>& Fixed<P, Rep>::operator+=(Fixed other) noexcept
{
    *this = *this + other;
    return *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep>& Fixed<P, Rep>::operator-=(Fixed other) noexcept
{
    *this = *this - other;
    return *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::operator-() const noexcept
{
    const Rep limit = std::numeric_limits<Rep>::max();
    const Rep clamped = x_ < -limit ? -limit : x_;
    return fromRaw(static_cast<Rep>(-clamped));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::operator*(Rep value) const noexcept
{
    return fromRaw(static_cast<Rep>(x_ * value));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::operator/(Rep value) const noexcept
{
    assert(value != 0);
    return fromRaw(static_cast<Rep>(x_ / value));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::abs() const noexcept
{
    return (*this < Rep{0}) ? -(*this) : *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::round() const noexcept
{
    using Unsigned = std::make_unsigned_t<Rep>;
    constexpr Unsigned HALF = Unsigned{1} << (P - 1);
    constexpr Unsigned MASK = static_cast<Unsigned>(~Unsigned{0} << P);
    const Unsigned rounded = (static_cast<Unsigned>(x_) + HALF) & MASK;
    return fromRaw(static_cast<Rep>(rounded));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::floor() const noexcept
{
    using Unsigned = std::make_unsigned_t<Rep>;
    constexpr Unsigned MASK = static_cast<Unsigned>(~Unsigned{0} << P);
    return fromRaw(static_cast<Rep>(static_cast<Unsigned>(x_) & MASK));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::ceil() const noexcept
{
    return -((-(*this)).floor());
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep>& Fixed<P, Rep>::operator*=(Fixed other) noexcept
{
    *this = (*this) * other;
    return *this;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::square() const noexcept
{
    const Fixed product = (*this) * (*this);
    assert(product.x_ >= 0);
    return product;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::inverse() const noexcept
{
    return Fixed{Rep{1}} / (*this);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::sqrt() const noexcept
{
    assert(x_ >= 0);
    Fixed result = fromRaw(static_cast<Rep>((x_ + (Rep{1} << PRECISION)) >> 1));
    for (int i = 0; i < PRECISION; ++i)
    {
        result = fromRaw(static_cast<Rep>((result.x_ + ((*this) / result).x_) >> 1));
    }
    return result;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::invSqrt() const noexcept
{
    assert(x_ >= 0);
    return Fixed{Rep{1}} / sqrt();
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::inverseApprox() const noexcept
{
    return inverse();
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::sqrtApprox() const noexcept
{
    assert(x_ >= 0);
    Fixed result = fromRaw(static_cast<Rep>((x_ + (Rep{1} << PRECISION)) >> 1));
    for (int i = 0; i < PRECISION / 2; ++i)
    {
        result = fromRaw(static_cast<Rep>((result.x_ + ((*this) / result).x_) >> 1));
    }
    return result;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::invSqrtApprox() const noexcept
{
    assert(x_ >= 0);
    return Fixed{Rep{1}} / sqrtApprox();
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::scaleByPowerOfTwo(int exp) const noexcept
{
    using Wide = detail::WideningT<Rep>;
    if (exp >= 0)
    {
        const Wide scaled = static_cast<Wide>(x_) << exp;
        assert(scaled == static_cast<Rep>(scaled));
        return fromRaw(static_cast<Rep>(scaled));
    }

    return fromRaw(static_cast<Rep>(x_ >> -exp));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::sin() const noexcept
{
    Rep sin_raw{};
    Rep cos_raw{};
    detail::cordicSinCos<P, Rep, CORDIC_ITERS>(x_, sin_raw, cos_raw);
    (void)cos_raw;
    return fromRaw(sin_raw);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::cos() const noexcept
{
    Rep sin_raw{};
    Rep cos_raw{};
    detail::cordicSinCos<P, Rep, CORDIC_ITERS>(x_, sin_raw, cos_raw);
    (void)sin_raw;
    return fromRaw(cos_raw);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr void Fixed<P, Rep>::sinCos(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    Rep sin_raw{};
    Rep cos_raw{};
    detail::cordicSinCos<P, Rep, CORDIC_ITERS>(x_, sin_raw, cos_raw);
    sin_result = fromRaw(sin_raw);
    cos_result = fromRaw(cos_raw);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::sinApprox() const noexcept
{
    Rep sin_raw{};
    Rep cos_raw{};
    detail::cordicSinCos<P, Rep, CORDIC_APPROX_ITERS>(x_, sin_raw, cos_raw);
    (void)cos_raw;
    return fromRaw(sin_raw);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr Fixed<P, Rep> Fixed<P, Rep>::cosApprox() const noexcept
{
    Rep sin_raw{};
    Rep cos_raw{};
    detail::cordicSinCos<P, Rep, CORDIC_APPROX_ITERS>(x_, sin_raw, cos_raw);
    (void)sin_raw;
    return fromRaw(cos_raw);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
constexpr void Fixed<P, Rep>::sinCosApprox(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    Rep sin_raw{};
    Rep cos_raw{};
    detail::cordicSinCos<P, Rep, CORDIC_APPROX_ITERS>(x_, sin_raw, cos_raw);
    sin_result = fromRaw(sin_raw);
    cos_result = fromRaw(cos_raw);
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline Fixed<P, Rep> Fixed<P, Rep>::sinViaFloat() const noexcept
{
    return fromRaw(detail::floatToFixed<Rep>(std::sin(getFloat()), P));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline Fixed<P, Rep> Fixed<P, Rep>::cosViaFloat() const noexcept
{
    return fromRaw(detail::floatToFixed<Rep>(std::cos(getFloat()), P));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
inline void Fixed<P, Rep>::sinCosViaFloat(Fixed& sin_result, Fixed& cos_result) const noexcept
{
    sin_result = fromRaw(detail::floatToFixed<Rep>(std::sin(getFloat()), P));
    cos_result = fromRaw(detail::floatToFixed<Rep>(std::cos(getFloat()), P));
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
[[nodiscard]]
constexpr Fixed<P, Rep> operator-(Rep value, Fixed<P, Rep> fixed) noexcept
{
    return Fixed<P, Rep>{value} - fixed;
}

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
[[nodiscard]]
constexpr Fixed<P, Rep> operator*(Rep value, Fixed<P, Rep> fixed) noexcept
{
    return fixed * value;
}


namespace std
{

template <int P, detail::FixedRep Rep>
    requires(P > 0 && P < static_cast<int>(sizeof(Rep) * 8))
class numeric_limits<Fixed<P, Rep>>
{
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = false;
    static constexpr bool is_exact = true;
    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr float_denorm_style has_denorm = denorm_absent;
    static constexpr bool has_denorm_loss = false;
    static constexpr float_round_style round_style = round_toward_zero;
    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;
    static constexpr int digits = numeric_limits<Rep>::digits;
    static constexpr int digits10 = numeric_limits<Rep>::digits10;
    static constexpr int max_digits10 = 0;
    static constexpr int radix = 2;
    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;
    static constexpr bool traps = false;
    static constexpr bool tinyness_before = false;

    [[nodiscard]]
    static constexpr Fixed<P, Rep> min() noexcept
    {
        return Fixed<P, Rep>::getMinValue();
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> lowest() noexcept
    {
        return Fixed<P, Rep>::getMinValue();
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> max() noexcept
    {
        return Fixed<P, Rep>::getMaxValue();
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> epsilon() noexcept
    {
        return Fixed<P, Rep>::getEpsilon();
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> round_error() noexcept
    {
        return Fixed<P, Rep>::fromRaw(Rep{1} << (P - 1));
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> infinity() noexcept
    {
        return Fixed<P, Rep>{};
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> quiet_NaN() noexcept
    {
        return Fixed<P, Rep>{};
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> signaling_NaN() noexcept
    {
        return Fixed<P, Rep>{};
    }

    [[nodiscard]]
    static constexpr Fixed<P, Rep> denorm_min() noexcept
    {
        return Fixed<P, Rep>::getEpsilon();
    }
};

} // namespace std
