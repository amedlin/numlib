#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include "fixedpt/fixed_types.h"
#include "int/isqrt.h"

namespace
{

template <typename FixedType>
[[nodiscard]]
float sumTolerance()
{
    return 10.0f * FixedType::getEpsilon().getFloat();
}

template <typename FixedType>
[[nodiscard]]
float productTolerance()
{
    return 2.0f * std::sqrt(FixedType::getEpsilon().getFloat());
}

template <typename FixedType>
[[nodiscard]]
float trigTolerance()
{
    return FixedType::PRECISION == 8 ? 0.04f : 0.004f;
}

template <typename FixedType>
void requireAbs(float x)
{
    const FixedType result = FixedType(x).abs();
    REQUIRE(std::fabs(result.getFloat() - std::fabs(x)) < sumTolerance<FixedType>());
}

template <typename FixedType>
void requireRound(float x)
{
    // Fixed::round matches floor(x + 0.5): halfway cases round toward +infinity.
    const FixedType result = FixedType(x).round();
    REQUIRE(std::fabs(result.getFloat() - std::floor(x + 0.5f)) < sumTolerance<FixedType>());
}

template <typename FixedType>
void requireFloor(float x)
{
    const FixedType result = FixedType(x).floor();
    REQUIRE(std::fabs(result.getFloat() - std::floor(x)) < sumTolerance<FixedType>());
}

template <typename FixedType>
void requireCeil(float x)
{
    const FixedType result = FixedType(x).ceil();
    REQUIRE(std::fabs(result.getFloat() - std::ceil(x)) < sumTolerance<FixedType>());
}

template <typename FixedType>
void requireAddition(float x1, float x2)
{
    const FixedType sum = FixedType(x1) + FixedType(x2);
    REQUIRE(std::fabs(sum.getFloat() - static_cast<float>(x1 + x2)) < sumTolerance<FixedType>());
}

template <typename FixedType>
void requireSubtraction(float x1, float x2)
{
    const FixedType difference = FixedType(x1) - FixedType(x2);
    REQUIRE(std::fabs(difference.getFloat() - static_cast<float>(x1 - x2)) < sumTolerance<FixedType>());
}

template <typename FixedType>
void requireMultiplication(float x1, float x2)
{
    const FixedType product = FixedType(x1) * FixedType(x2);
    REQUIRE(std::fabs(product.getFloat() / (x1 * x2) - 1.0f) < productTolerance<FixedType>());
}

template <typename FixedType>
void requireDivision(float x1, float x2)
{
    REQUIRE(x2 != 0.0f);
    const float ratio_float = x1 / x2;
    REQUIRE(std::fabs(ratio_float) >= FixedType::getEpsilon().getFloat());
    REQUIRE(std::fabs(ratio_float) >= 10.0f * FixedType::getEpsilon().getFloat());
    REQUIRE(std::fabs(ratio_float) <= FixedType::getMaxValue().getFloat());

    const FixedType ratio = FixedType(x1) / FixedType(x2);
    REQUIRE(std::fabs(ratio.getFloat() / ratio_float - 1.0f) < productTolerance<FixedType>());
}

template <typename FixedType>
void requireNotValidDivision(float x1, float x2)
{
    REQUIRE(x2 != 0.0f);
    const float ratio_float = x1 / x2;
    if (std::fabs(ratio_float) < FixedType::getEpsilon().getFloat() ||
        std::fabs(ratio_float) < 10.0f * FixedType::getEpsilon().getFloat() ||
        std::fabs(ratio_float) > FixedType::getMaxValue().getFloat())
    {
        SUCCEED("Division is expected to be invalid for this input");
        return;
    }

    const FixedType ratio = FixedType(x1) / FixedType(x2);
    REQUIRE(std::fabs(ratio.getFloat() / ratio_float - 1.0f) >= productTolerance<FixedType>());
}

template <typename FixedType>
void requireSquare(float x)
{
    REQUIRE(x * x <= FixedType::getMaxValue().getFloat());
    if (x != 0.0f)
    {
        REQUIRE(x * x >= 10.0f * FixedType::getEpsilon().getFloat());
    }

    const FixedType square = FixedType(x).square();
    if (x != 0.0f)
    {
        REQUIRE(std::fabs(square.getFloat() / (x * x) - 1.0f) < productTolerance<FixedType>());
    }
    else
    {
        REQUIRE(square.getFloat() == 0.0f);
    }
}

template <typename FixedType>
void requireInverse(float x)
{
    REQUIRE(std::fabs(x) >= FixedType::getEpsilon().getFloat());
    const FixedType inv = FixedType(x).inverse();
    REQUIRE(std::fabs(inv.getFloat() * x - 1.0f) < productTolerance<FixedType>());
}

template <typename FixedType>
void requireNotValidInverse(float x)
{
    if (std::fabs(x) < FixedType::getEpsilon().getFloat())
    {
        SUCCEED("Inverse is expected to be invalid for this input");
        return;
    }

    const FixedType inv = FixedType(x).inverse();
    REQUIRE(std::fabs(inv.getFloat() * x - 1.0f) >= productTolerance<FixedType>());
}

template <typename FixedType>
void requireSqrt(float x)
{
    REQUIRE(x >= 0.0f);
    const FixedType root = FixedType(x).sqrt();
    if (x == 0.0f)
    {
        REQUIRE(root.getFloat() == 0.0f);
    }
    else
    {
        REQUIRE(std::fabs(root.getFloat() / std::sqrt(x) - 1.0f) < productTolerance<FixedType>());
    }
}

template <typename FixedType>
void requireInvSqrt(float x)
{
    REQUIRE(x >= 0.0f);
    const FixedType inv_root = FixedType(x).invSqrt();
    REQUIRE(std::fabs(inv_root.getFloat() * std::sqrt(x) - 1.0f) < productTolerance<FixedType>());
}

template <typename FixedType>
void requireSin(float x)
{
    const FixedType result = FixedType(x).sin();
    REQUIRE(std::fabs(result.getFloat() - std::sin(x)) < trigTolerance<FixedType>());
}

template <typename FixedType>
void requireCos(float x)
{
    const FixedType result = FixedType(x).cos();
    REQUIRE(std::fabs(result.getFloat() - std::cos(x)) < trigTolerance<FixedType>());
}

template <typename FixedType>
void requireSinCos(float x)
{
    FixedType sin_result;
    FixedType cos_result;
    FixedType(x).sinCos(sin_result, cos_result);
    REQUIRE(std::fabs(sin_result.getFloat() - std::sin(x)) < trigTolerance<FixedType>());
    REQUIRE(std::fabs(cos_result.getFloat() - std::cos(x)) < trigTolerance<FixedType>());
}

template <typename FixedType>
void requireInverseApprox(float x)
{
    const FixedType inv = FixedType(x).inverseApprox();
    REQUIRE(std::fabs(inv.getFloat() * x - 1.0f) < 10.0f * productTolerance<FixedType>());
}

template <typename FixedType>
void requireSqrtApprox(float x)
{
    REQUIRE(x >= 0.0f);
    const FixedType root = FixedType(x).sqrtApprox();
    if (x == 0.0f)
    {
        REQUIRE(root.getFloat() == 0.0f);
    }
    else
    {
        REQUIRE(std::fabs(root.getFloat() / std::sqrt(x) - 1.0f) < 10.0f * productTolerance<FixedType>());
    }
}

template <typename FixedType>
void requireInvSqrtApprox(float x)
{
    REQUIRE(x >= 0.0f);
    const FixedType inv_root = FixedType(x).invSqrtApprox();
    REQUIRE(std::fabs(inv_root.getFloat() * std::sqrt(x) - 1.0f) < 10.0f * productTolerance<FixedType>());
}

template <typename FixedType>
void requireSinApprox(float x)
{
    const FixedType result = FixedType(x).sinApprox();
    REQUIRE(std::fabs(result.getFloat() - std::sin(x)) < 10.0f * trigTolerance<FixedType>());
}

template <typename FixedType>
void requireCosApprox(float x)
{
    const FixedType result = FixedType(x).cosApprox();
    REQUIRE(std::fabs(result.getFloat() - std::cos(x)) < 10.0f * trigTolerance<FixedType>());
}

template <typename FixedType>
void requireSinCosApprox(float x)
{
    FixedType sin_result;
    FixedType cos_result;
    FixedType(x).sinCosApprox(sin_result, cos_result);
    REQUIRE(std::fabs(sin_result.getFloat() - std::sin(x)) < 10.0f * trigTolerance<FixedType>());
    REQUIRE(std::fabs(cos_result.getFloat() - std::cos(x)) < 10.0f * trigTolerance<FixedType>());
}

void requireExpApprox(float x)
{
    const Fixed16 result = expApprox(Fixed16(x));
    REQUIRE(std::fabs(result.getFloat() - std::exp(x)) < productTolerance<Fixed16>());
}

} // namespace


TEST_CASE("Fixed construction from ratio and conversions")
{
    const Fixed16 fixed_16(-6, 8);
    REQUIRE(fixed_16.getFloat() == -0.75f);

    const FixedI fixed_i = convert<8>(fixed_16);
    REQUIRE(fixed_i.getFloat() == -0.75f);

    const FixedI fixed_i_ratio(-6, 8);
    REQUIRE(fixed_i_ratio.getFloat() == -0.75f);

    const FixedF fixed_f = convert<24>(fixed_16);
    REQUIRE(std::fabs(fixed_f.getFloat() - (-0.75f)) < productTolerance<FixedF>());
    REQUIRE(std::fabs(convert<16>(fixed_f).getFloat() - (-0.75f)) < productTolerance<Fixed16>());
}

TEST_CASE("Fixed assignment from int, float, and double")
{
    Fixed16 fixed_16;
    FixedI fixed_i;
    FixedF fixed_f;

    const std::int32_t i0 = 5;
    const std::int32_t i1 = -3;
    fixed_16 = i0;
    REQUIRE(fixed_16.getInt() == i0);
    fixed_f = i0;
    REQUIRE(fixed_f.getInt() == i0);
    fixed_i = i0;
    REQUIRE(fixed_i.getInt() == i0);
    fixed_16 = i1;
    REQUIRE(fixed_16.getInt() == i1);
    fixed_f = i1;
    REQUIRE(fixed_f.getInt() == i1);
    fixed_i = i1;
    REQUIRE(fixed_i.getInt() == i1);

    const float f0 = 3.5f;
    const float f1 = -11.8f;
    fixed_16 = f0;
    REQUIRE(std::fabs(fixed_16.getFloat() - f0) <= Fixed16::getEpsilon().getFloat());
    fixed_f = f0;
    REQUIRE(std::fabs(fixed_f.getFloat() - f0) <= FixedF::getEpsilon().getFloat());
    fixed_i = f0;
    REQUIRE(std::fabs(fixed_i.getFloat() - f0) <= FixedI::getEpsilon().getFloat());
    fixed_16 = f1;
    REQUIRE(std::fabs(fixed_16.getFloat() - f1) <= Fixed16::getEpsilon().getFloat());
    fixed_f = f1;
    REQUIRE(std::fabs(fixed_f.getFloat() - f1) <= FixedF::getEpsilon().getFloat());
    fixed_i = f1;
    REQUIRE(std::fabs(fixed_i.getFloat() - f1) <= FixedI::getEpsilon().getFloat());

    const double d0 = 1.6;
    const double d1 = -6.3;
    fixed_16 = d0;
    REQUIRE(std::fabs(fixed_16.getFloat() - static_cast<float>(d0)) <= Fixed16::getEpsilon().getFloat());
    fixed_f = d0;
    REQUIRE(std::fabs(fixed_f.getFloat() - static_cast<float>(d0)) <= FixedF::getEpsilon().getFloat());
    fixed_i = d0;
    REQUIRE(std::fabs(fixed_i.getFloat() - static_cast<float>(d0)) <= FixedI::getEpsilon().getFloat());
    fixed_16 = d1;
    REQUIRE(std::fabs(fixed_16.getFloat() - static_cast<float>(d1)) <= Fixed16::getEpsilon().getFloat());
    fixed_f = d1;
    REQUIRE(std::fabs(fixed_f.getFloat() - static_cast<float>(d1)) <= FixedF::getEpsilon().getFloat());
    fixed_i = d1;
    REQUIRE(std::fabs(fixed_i.getFloat() - static_cast<float>(d1)) <= FixedI::getEpsilon().getFloat());
}

TEST_CASE("Fixed abs, round, floor, and ceil")
{
    constexpr float x1 = 102.583f;
    constexpr float x2 = 0.038f;
    constexpr float x3 = -900.2f;
    constexpr float x4 = -311.2f;
    constexpr float x5 = 27834.9784f;
    constexpr float x6 = -4.79f;
    constexpr float x7 = 25.99f;
    constexpr float x8 = 0.38f;
    constexpr float x9 = 5.5f;
    constexpr float x10 = -8.5f;

    requireAbs<FixedF>(x1);
    requireAbs<FixedF>(x2);
    requireAbs<Fixed16>(x3);
    requireAbs<Fixed16>(x4);
    requireAbs<FixedI>(x5);
    requireAbs<FixedI>(x6);
    requireAbs<Fixed16>(x7);
    requireAbs<Fixed16>(x8);

    requireRound<FixedF>(x1);
    requireRound<FixedF>(x2);
    requireRound<Fixed16>(x3);
    requireRound<Fixed16>(x4);
    requireRound<FixedI>(x5);
    requireRound<FixedI>(x6);
    requireRound<Fixed16>(x7);
    requireRound<Fixed16>(x8);
    requireRound<FixedF>(x9);
    requireRound<FixedF>(x10);
    requireRound<Fixed16>(x9);
    requireRound<Fixed16>(x10);
    requireRound<FixedI>(x9);
    requireRound<FixedI>(x10);

    requireFloor<Fixed16>(x1);
    requireFloor<Fixed16>(x2);
    requireFloor<Fixed16>(x3);
    requireFloor<Fixed16>(x4);
    requireFloor<Fixed16>(x5);
    requireFloor<Fixed16>(x6);
    requireFloor<Fixed16>(x7);
    requireFloor<Fixed16>(x8);
    requireFloor<FixedF>(x1);
    requireFloor<FixedF>(x2);
    requireFloor<FixedF>(x6);
    requireFloor<FixedF>(x7);
    requireFloor<FixedF>(x8);
    requireFloor<FixedI>(x1);
    requireFloor<FixedI>(x3);
    requireFloor<FixedI>(x4);
    requireFloor<FixedI>(x5);
    requireFloor<FixedI>(x6);
    requireFloor<FixedI>(x7);
    requireFloor<FixedI>(x8);

    requireCeil<Fixed16>(x1);
    requireCeil<Fixed16>(x2);
    requireCeil<Fixed16>(x3);
    requireCeil<Fixed16>(x4);
    requireCeil<Fixed16>(x5);
    requireCeil<Fixed16>(x6);
    requireCeil<Fixed16>(x7);
    requireCeil<Fixed16>(x8);
    requireCeil<FixedF>(x1);
    requireCeil<FixedF>(x2);
    requireCeil<FixedF>(x6);
    requireCeil<FixedF>(x7);
    requireCeil<FixedF>(x8);
    requireCeil<FixedI>(x1);
    requireCeil<FixedI>(x3);
    requireCeil<FixedI>(x4);
    requireCeil<FixedI>(x5);
    requireCeil<FixedI>(x6);
    requireCeil<FixedI>(x7);
    requireCeil<FixedI>(x8);
}

TEST_CASE("Fixed addition and subtraction")
{
    constexpr float x1 = 102.583f;
    constexpr float x2 = 0.038f;
    constexpr float x3 = -900.2f;
    constexpr float x4 = -311.2f;
    constexpr float x6 = -4.79f;

    requireAddition<Fixed16>(x1, x2);
    requireAddition<Fixed16>(x1, x3);
    requireAddition<Fixed16>(x2, x3);
    requireAddition<FixedI>(x1, x2);
    requireAddition<FixedI>(x1, x3);
    requireAddition<FixedI>(x3, x4);
    requireAddition<FixedF>(x1, x2);
    requireAddition<FixedF>(x1, x6);
    requireAddition<FixedF>(x2, x6);

    requireSubtraction<Fixed16>(x1, x2);
    requireSubtraction<Fixed16>(x1, x3);
    requireSubtraction<Fixed16>(x2, x3);
    requireSubtraction<FixedI>(x1, x2);
    requireSubtraction<FixedI>(x1, x3);
    requireSubtraction<FixedI>(x3, x4);
    requireSubtraction<FixedF>(x1, x2);
    requireSubtraction<FixedF>(x1, x6);
    requireSubtraction<FixedF>(x2, x6);
}

TEST_CASE("Fixed multiplication and division")
{
    constexpr float x1 = 102.583f;
    constexpr float x2 = 0.038f;
    constexpr float x3 = -900.2f;
    constexpr float x4 = -311.2f;
    constexpr float x6 = -4.79f;
    constexpr float x7 = 25.99f;

    requireMultiplication<Fixed16>(x1, x2);
    requireMultiplication<Fixed16>(x1, x4);
    requireMultiplication<Fixed16>(x2, x4);
    requireMultiplication<FixedI>(x1, x2);
    requireMultiplication<FixedI>(x1, x4);
    requireMultiplication<FixedI>(x2, x4);
    requireMultiplication<FixedF>(x1, x2);
    requireMultiplication<FixedF>(x6, x7);
    requireMultiplication<FixedF>(x2, x6);

    requireDivision<Fixed16>(x1, x4);
    requireNotValidDivision<Fixed16>(x2, x3);
    requireDivision<Fixed16>(x3, x4);
    requireDivision<FixedI>(x1, x4);
    requireNotValidDivision<FixedI>(x2, x3);
    requireDivision<FixedI>(x3, x4);
    requireDivision<FixedF>(x2, x1);
    requireNotValidDivision<FixedF>(x7, x2);
    requireDivision<FixedF>(x6, x2);
}

TEST_CASE("Fixed square, inverse, sqrt, and invSqrt")
{
    constexpr float x1 = 102.583f;
    constexpr float x2 = 0.038f;
    constexpr float x3 = -900.2f;
    constexpr float x4 = -311.2f;
    constexpr float x5 = 27834.9784f;
    constexpr float x6 = -4.79f;
    constexpr float x7 = 25.99f;
    constexpr float x8 = 0.38f;

    requireSquare<Fixed16>(0.0f);
    requireSquare<Fixed16>(x1);
    requireSquare<Fixed16>(x7);
    requireSquare<Fixed16>(x8);
    requireSquare<FixedI>(0.0f);
    requireSquare<FixedI>(x1);
    requireSquare<FixedI>(x3);
    requireSquare<FixedI>(x8);
    requireSquare<FixedF>(0.0f);
    requireSquare<FixedF>(x2);
    requireSquare<FixedF>(x8);
    requireSquare<FixedF>(x4 / x3);

    requireInverse<Fixed16>(1.0f);
    requireInverse<Fixed16>(x1);
    requireInverse<Fixed16>(x2);
    requireNotValidInverse<Fixed16>(x3);
    requireInverse<Fixed16>(x4);
    requireInverse<FixedI>(1.0f);
    requireNotValidInverse<FixedI>(x1);
    requireInverse<FixedI>(x2);
    requireNotValidInverse<FixedI>(x3);
    requireInverse<FixedI>(x6);
    requireInverse<FixedF>(1.0f);
    requireInverse<FixedF>(x1);
    requireInverse<FixedF>(x6);
    requireInverse<FixedF>(x7);
    requireInverse<FixedF>(x8);

    requireSqrt<Fixed16>(0.0f);
    requireSqrt<Fixed16>(1.0f);
    requireSqrt<Fixed16>(x1);
    requireSqrt<Fixed16>(x2);
    requireSqrt<Fixed16>(x5);
    requireSqrt<FixedI>(0.0f);
    requireSqrt<FixedI>(1.0f);
    requireSqrt<FixedI>(x1);
    requireSqrt<FixedI>(x5);
    requireSqrt<FixedI>(x7);
    requireSqrt<FixedF>(0.0f);
    requireSqrt<FixedF>(1.0f);
    requireSqrt<FixedF>(x1);
    requireSqrt<FixedF>(x7);
    requireSqrt<FixedF>(x8);

    requireInvSqrt<Fixed16>(1.0f);
    requireInvSqrt<Fixed16>(x1);
    requireInvSqrt<Fixed16>(x2);
    requireInvSqrt<Fixed16>(x5);
    requireInvSqrt<FixedI>(1.0f);
    requireInvSqrt<FixedI>(x1);
    requireInvSqrt<FixedI>(x2);
    requireInvSqrt<FixedI>(x7);
    requireInvSqrt<FixedF>(1.0f);
    requireInvSqrt<FixedF>(x1);
    requireInvSqrt<FixedF>(x2);
    requireInvSqrt<FixedF>(x8);
}

TEST_CASE("Fixed trigonometric functions")
{
    constexpr float x1 = 102.583f;
    constexpr float x2 = 0.038f;
    constexpr float x3 = -900.2f;
    constexpr float x4 = -311.2f;
    constexpr float x5 = 27834.9784f;
    constexpr float x6 = -4.79f;
    constexpr float x7 = 25.99f;
    constexpr float x8 = 0.38f;

    requireSin<Fixed16>(x1);
    requireSin<Fixed16>(x2);
    requireSin<Fixed16>(x3);
    requireSin<Fixed16>(x4);
    requireSin<Fixed16>(x5);
    requireCos<Fixed16>(x1);
    requireCos<Fixed16>(x2);
    requireCos<Fixed16>(x3);
    requireCos<Fixed16>(x4);
    requireCos<Fixed16>(x5);
    requireSinCos<Fixed16>(x1);
    requireSinCos<Fixed16>(x2);
    requireSinCos<Fixed16>(x3);
    requireSinCos<Fixed16>(x4);
    requireSinCos<Fixed16>(x5);

    requireSin<FixedI>(x1);
    requireSin<FixedI>(x2);
    requireSin<FixedI>(x3);
    requireSin<FixedI>(x4);
    requireSin<FixedI>(x5);
    requireCos<FixedI>(x1);
    requireCos<FixedI>(x2);
    requireCos<FixedI>(x3);
    requireCos<FixedI>(x4);
    requireCos<FixedI>(x5);
    requireSinCos<FixedI>(x1);
    requireSinCos<FixedI>(x2);
    requireSinCos<FixedI>(x3);
    requireSinCos<FixedI>(x4);
    requireSinCos<FixedI>(x5);

    requireSin<FixedF>(x1);
    requireSin<FixedF>(x2);
    requireSin<FixedF>(x6);
    requireSin<FixedF>(x7);
    requireSin<FixedF>(x8);
    requireCos<FixedF>(x1);
    requireCos<FixedF>(x2);
    requireCos<FixedF>(x6);
    requireCos<FixedF>(x7);
    requireCos<FixedF>(x8);
    requireSinCos<FixedF>(x1);
    requireSinCos<FixedF>(x2);
    requireSinCos<FixedF>(x6);
    requireSinCos<FixedF>(x7);
    requireSinCos<FixedF>(x8);
}

TEST_CASE("Fixed type specializations and mixed operators")
{
    constexpr float f0 = -107.239f;
    constexpr float f1 = -18570.239f;
    constexpr float f2 = -5.324f;
    constexpr float f3 = 0.875403f;
    constexpr float f4 = 4782800.23f;
    constexpr float f5 = -3490.88f;

    const Fixed16 x_x1(f0);
    const Fixed16 x_x2(f1);
    const FixedF x_f1(f2);
    const FixedF x_f2(f3);
    const FixedI x_i1(f4);
    const FixedI x_i2(f5);

    REQUIRE(std::fabs(mulAs<16>(x_x1, x_f1).getFloat() - f0 * f2) < productTolerance<Fixed16>());
    REQUIRE(std::fabs(mulAs<16>(x_x1, x_f2).getFloat() - f0 * f3) < productTolerance<Fixed16>());
    REQUIRE(std::fabs(mulAs<16>(x_x2, x_f2).getFloat() - f1 * f3) < productTolerance<Fixed16>());

    Fixed16 temp_x = convert<16>(x_f1);
    REQUIRE(std::fabs(temp_x.getFloat() - f2) < productTolerance<Fixed16>());

    FixedF temp_f = convert<24>(x_x1);
    REQUIRE(std::fabs(temp_f.getFloat() - f0) < productTolerance<FixedF>());

    REQUIRE(std::fabs(mulAs<16>(x_f1, x_i2).getFloat() - f2 * f5) < productTolerance<FixedI>());
    REQUIRE(std::fabs(mulAs<16>(x_f2, x_i2).getFloat() - f3 * f5) < productTolerance<FixedI>());

    REQUIRE(std::fabs(mulAs<16>(x_f1, x_x1).getFloat() - f2 * f0) < productTolerance<Fixed16>());
    REQUIRE(std::fabs(mulAs<16>(x_f2, x_x1).getFloat() - f3 * f0) < productTolerance<Fixed16>());
    REQUIRE(std::fabs(mulAs<16>(x_f2, x_x2).getFloat() - f3 * f1) < productTolerance<Fixed16>());

    REQUIRE(std::fabs(divAs<24>(x_f1, x_i1).getFloat() - f2 / f4) < productTolerance<FixedF>());
    REQUIRE(std::fabs(divAs<24>(x_f1, x_i2).getFloat() - f2 / f5) < productTolerance<FixedF>());
    REQUIRE(std::fabs(divAs<24>(x_f2, x_i1).getFloat() - f3 / f4) < productTolerance<FixedF>());
    REQUIRE(std::fabs(divAs<24>(x_f2, x_i2).getFloat() - f3 / f5) < productTolerance<FixedF>());

    FixedI temp_i = convert<8>(x_x1);
    REQUIRE(std::fabs(temp_i.getFloat() - f0) < productTolerance<FixedI>());
    temp_i = convert<8>(x_x2);
    REQUIRE(std::fabs(temp_i.getFloat() - f1) < productTolerance<FixedI>());

    REQUIRE(std::fabs(mulAs<16>(x_i2, x_f1).getFloat() - f5 * f2) < productTolerance<FixedI>());
    REQUIRE(std::fabs(mulAs<16>(x_i2, x_f2).getFloat() - f5 * f3) < productTolerance<FixedI>());

    REQUIRE(std::fabs(mulAs<8>(x_i2, x_x1).getFloat() - f5 * f0) < 5.0f * productTolerance<FixedI>());
    REQUIRE(std::fabs(mulAs<8>(x_x1, x_i2).getFloat() - f5 * f0) < 5.0f * productTolerance<FixedI>());

    REQUIRE(std::fabs(divAs<8>(x_i2, x_x1).getFloat() - f5 / f0) < productTolerance<FixedI>());
    REQUIRE(std::fabs(divAs<8>(x_i1, x_x2).getFloat() - f4 / f1) < productTolerance<FixedI>());

    constexpr int numerator1 = 4;
    constexpr int denominator1 = 7;
    constexpr int numerator2 = -11;
    constexpr int denominator2 = 9;
    const FixedF ratio_f1(numerator1, denominator1);
    REQUIRE(std::fabs(ratio_f1.getFloat() - static_cast<float>(numerator1) / static_cast<float>(denominator1)) <
            productTolerance<FixedF>());
    const FixedF ratio_f2(numerator2, denominator2);
    REQUIRE(std::fabs(ratio_f2.getFloat() - static_cast<float>(numerator2) / static_cast<float>(denominator2)) <
            productTolerance<FixedF>());

    REQUIRE(std::fabs((x_x1 * numerator1).getFloat() - f0 * numerator1) < productTolerance<Fixed16>());
    REQUIRE(std::fabs((x_f2 * denominator1).getFloat() - f3 * denominator1) < productTolerance<FixedF>());
    REQUIRE(std::fabs((x_i2 * numerator2).getFloat() - f5 * numerator2) < productTolerance<FixedI>());

    REQUIRE(std::fabs((x_x2 / numerator1).getFloat() - f1 / numerator1) < productTolerance<Fixed16>());
    REQUIRE(std::fabs((x_f1 / denominator1).getFloat() - f2 / denominator1) < productTolerance<FixedF>());
    REQUIRE(std::fabs((x_i1 / numerator2).getFloat() - f4 / numerator2) < productTolerance<FixedI>());

    REQUIRE(std::fabs((numerator1 - x_x1).getFloat() - (numerator1 - f0)) < sumTolerance<Fixed16>());
    REQUIRE(std::fabs((denominator1 - x_f2).getFloat() - (denominator1 - f3)) < sumTolerance<FixedF>());
    REQUIRE(std::fabs((numerator2 - x_i2).getFloat() - (numerator2 - f5)) < sumTolerance<FixedI>());

    REQUIRE(std::fabs((numerator1 * x_x1).getFloat() - f0 * numerator1) < productTolerance<Fixed16>());
    REQUIRE(std::fabs((denominator1 * x_f2).getFloat() - f3 * denominator1) < productTolerance<FixedF>());
    REQUIRE(std::fabs((numerator2 * x_i2).getFloat() - f5 * numerator2) < productTolerance<FixedI>());

    temp_x = x_x1;
    REQUIRE(std::fabs((temp_x += x_x2).getFloat() - (f0 + f1)) < sumTolerance<Fixed16>());
    temp_f = x_f1;
    REQUIRE(std::fabs((temp_f += x_f2).getFloat() - (f2 + f3)) < sumTolerance<FixedF>());
    temp_i = x_i1;
    REQUIRE(std::fabs((temp_i += x_i2).getFloat() - (f4 + f5)) < sumTolerance<FixedI>());

    temp_x = x_x1;
    REQUIRE(std::fabs((temp_x -= x_x2).getFloat() - (f0 - f1)) < sumTolerance<Fixed16>());
    temp_f = x_f1;
    REQUIRE(std::fabs((temp_f -= x_f2).getFloat() - (f2 - f3)) < sumTolerance<FixedF>());
    temp_i = x_i1;
    REQUIRE(std::fabs((temp_i -= x_i2).getFloat() - (f4 - f5)) < sumTolerance<FixedI>());

    temp_x = x_x2;
    REQUIRE(std::fabs((temp_x *= Fixed16(f3)).getFloat() - f1 * f3) <
            productTolerance<Fixed16>() * std::fabs(f1 * f3));
    temp_f = x_f1;
    REQUIRE(std::fabs((temp_f *= FixedF(f3)).getFloat() - f2 * f3) <
            productTolerance<FixedF>() * std::fabs(f2 * f3));
    temp_i = x_i2;
    REQUIRE(std::fabs((temp_i *= FixedI(f0)).getFloat() - f5 * f0) <
            productTolerance<FixedI>() * std::fabs(f5 * f0));
}

TEST_CASE("Fixed approximate inverse, sqrt, invSqrt, trig, and exp")
{
    constexpr float x1 = 0.0401f;
    constexpr float x2 = 0.14f;
    constexpr float x3 = -5.2f;
    constexpr float x4 = 28.92f;
    constexpr float x5 = -684.92f;

    requireInverseApprox<Fixed16>(x1);
    requireInverseApprox<Fixed16>(x2);
    requireInverseApprox<Fixed16>(x3);
    requireInverseApprox<Fixed16>(x4);
    requireInverseApprox<Fixed16>(x5);
    requireInverseApprox<FixedI>(x1);
    requireInverseApprox<FixedI>(x2);
    requireInverseApprox<FixedI>(x3);
    requireInverseApprox<FixedI>(x4);
    requireInverseApprox<FixedF>(x1);
    requireInverseApprox<FixedF>(x2);
    requireInverseApprox<FixedF>(x3);
    requireInverseApprox<FixedF>(x4);

    requireSqrtApprox<Fixed16>(x1);
    requireSqrtApprox<Fixed16>(x2);
    requireSqrtApprox<Fixed16>(x4);
    requireSqrtApprox<FixedI>(x1);
    requireSqrtApprox<FixedI>(x2);
    requireSqrtApprox<FixedI>(x4);
    requireSqrtApprox<FixedF>(x1);
    requireSqrtApprox<FixedF>(x2);
    requireSqrtApprox<FixedF>(x4);

    requireInvSqrtApprox<Fixed16>(x1);
    requireInvSqrtApprox<Fixed16>(x2);
    requireInvSqrtApprox<Fixed16>(x4);
    requireInvSqrtApprox<FixedI>(x1);
    requireInvSqrtApprox<FixedI>(x2);
    requireInvSqrtApprox<FixedI>(x4);
    requireInvSqrtApprox<FixedF>(x1);
    requireInvSqrtApprox<FixedF>(x2);
    requireInvSqrtApprox<FixedF>(x4);

    requireSinApprox<Fixed16>(x1);
    requireSinApprox<Fixed16>(x2);
    requireSinApprox<Fixed16>(x3);
    requireSinApprox<Fixed16>(x4);
    requireSinApprox<Fixed16>(x5);
    requireSinApprox<FixedI>(x1);
    requireSinApprox<FixedI>(x2);
    requireSinApprox<FixedI>(x3);
    requireSinApprox<FixedI>(x4);
    requireSinApprox<FixedI>(x5);
    requireSinApprox<FixedF>(x1);
    requireSinApprox<FixedF>(x2);
    requireSinApprox<FixedF>(x3);
    requireSinApprox<FixedF>(x4);

    requireCosApprox<Fixed16>(x1);
    requireCosApprox<Fixed16>(x2);
    requireCosApprox<Fixed16>(x3);
    requireCosApprox<Fixed16>(x4);
    requireCosApprox<Fixed16>(x5);
    requireCosApprox<FixedI>(x1);
    requireCosApprox<FixedI>(x2);
    requireCosApprox<FixedI>(x3);
    requireCosApprox<FixedI>(x4);
    requireCosApprox<FixedI>(x5);
    requireCosApprox<FixedF>(x1);
    requireCosApprox<FixedF>(x2);
    requireCosApprox<FixedF>(x3);
    requireCosApprox<FixedF>(x4);
    requireSinCosApprox<Fixed16>(x3);
    requireSinCosApprox<FixedI>(x3);
    requireSinCosApprox<FixedF>(x3);

    requireExpApprox(x1);
    requireExpApprox(-x1);
    requireExpApprox(x2);
    requireExpApprox(-x2);
    requireExpApprox(x3);
    requireExpApprox(-x3);
    requireExpApprox(-x4);
}

TEST_CASE("Fixed mulAdd, scaleByPowerOfTwo, ceil on integers, and numeric_limits")
{
    const Fixed16 a(3.0f);
    const Fixed16 b(4.0f);
    const Fixed16 c(5.0f);
    REQUIRE(std::fabs(mulAdd(a, b, c).getFloat() - 17.0f) < sumTolerance<Fixed16>());

    const Fixed16 eight(8.0f);
    REQUIRE(eight.scaleByPowerOfTwo(1).getFloat() == 16.0f);
    REQUIRE(eight.scaleByPowerOfTwo(-2).getFloat() == 2.0f);

    const Fixed16 exact(5);
    REQUIRE(exact.ceil().getFloat() == 5.0f);
    const Fixed16 inexact(5.25f);
    REQUIRE(inexact.ceil().getFloat() == 6.0f);

    using Limits = std::numeric_limits<Fixed16>;
    REQUIRE(Limits::is_specialized);
    REQUIRE(Limits::is_exact);
    REQUIRE_FALSE(Limits::is_integer);
    REQUIRE(Limits::epsilon().getRawValue() == 1);
    REQUIRE(Limits::max().getRawValue() == Fixed16::getMaxValue().getRawValue());
    REQUIRE(Limits::min().getRawValue() == Fixed16::getMinValue().getRawValue());
}

TEST_CASE("Fixed Rep parameter and viaFloat trig")
{
    using Q88 = Fixed<8, std::int16_t>;
    const Q88 x{std::int16_t{3}};
    const Q88 y{std::int16_t{2}};
    REQUIRE((x * y).getInt() == 6);
    REQUIRE(std::fabs(x.getFloat() - 3.0f) < Q88::getEpsilon().getFloat());

    const Fixed16 angle(0.5f);
    REQUIRE(std::fabs(angle.sinViaFloat().getFloat() - std::sin(0.5f)) < trigTolerance<Fixed16>());
    REQUIRE(std::fabs(angle.cosViaFloat().getFloat() - std::cos(0.5f)) < trigTolerance<Fixed16>());
}

namespace
{

template <typename FixedType>
void requireFixedSqrtRawIdentity(FixedType value)
{
    REQUIRE(value.getRawValue() >= 0);
    const FixedType via_method = value.sqrt();
    const FixedType via_free = integerSqrt(value);
    REQUIRE(via_method.getRawValue() == via_free.getRawValue());

    const auto raw = static_cast<std::uint32_t>(value.getRawValue());
    const std::uint64_t widened =
        static_cast<std::uint64_t>(raw) << FixedType::PRECISION;
    const std::uint64_t expected = integerSqrt(widened).p_;
    REQUIRE(static_cast<std::uint64_t>(via_method.getRawValue()) == expected);
}

template <typename FixedType>
void exerciseFixedSqrtComprehensive()
{
    requireFixedSqrtRawIdentity(FixedType{0});
    requireFixedSqrtRawIdentity(FixedType::getEpsilon());
    requireFixedSqrtRawIdentity(FixedType{1});
    requireFixedSqrtRawIdentity(FixedType{2});
    requireFixedSqrtRawIdentity(FixedType{0.25f});
    requireFixedSqrtRawIdentity(FixedType{0.5f});

    const FixedType max_value = FixedType::getMaxValue();
    if (max_value.getRawValue() > 0)
    {
        requireFixedSqrtRawIdentity(max_value);
    }

    std::mt19937 rng{0xF15EDULL};
    std::uniform_int_distribution<std::int32_t> dist{
        0,
        max_value.getRawValue()};

    std::vector<FixedType> samples;
    samples.reserve(512);
    for (int i = 0; i < 512; ++i)
    {
        const FixedType sample = FixedType::fromRaw(dist(rng));
        requireFixedSqrtRawIdentity(sample);
        samples.push_back(sample);
    }

    std::sort(
        samples.begin(),
        samples.end(),
        [](FixedType a, FixedType b)
        {
            return a.getRawValue() < b.getRawValue();
        });

    for (std::size_t i = 1; i < samples.size(); ++i)
    {
        REQUIRE(samples[i - 1].sqrt().getRawValue() <= samples[i].sqrt().getRawValue());
    }

    // Algebra smoke: sqrt(x*x) ~= |x| when x*x stays in range.
    for (float x : {0.0f, 0.5f, 1.0f, 2.0f, 3.5f, 10.0f})
    {
        const FixedType value{x};
        const FixedType squared = value * value;
        if (squared.getRawValue() < 0)
        {
            continue;
        }
        const FixedType root = squared.sqrt();
        REQUIRE(std::fabs(root.getFloat() - x) < 4.0f * productTolerance<FixedType>());
    }
}

} // namespace

TEST_CASE("Fixed integerSqrt matches sqrt and floor(sqrt(raw<<P))")
{
    exerciseFixedSqrtComprehensive<Fixed16>();
    exerciseFixedSqrtComprehensive<FixedF>();
    exerciseFixedSqrtComprehensive<FixedI>();
}
