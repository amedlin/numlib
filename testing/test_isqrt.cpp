#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <random>

#include "int/isqrt.h"

TEST_CASE("integerSqrt handles non-positive inputs")
{
    const IntSqrtResult negative = integerSqrt(-7);
    REQUIRE(negative.p_ == 0);
    REQUIRE(negative.q_ == -7);

    const IntSqrtResult zero = integerSqrt(0);
    REQUIRE(zero.p_ == 0);
    REQUIRE(zero.q_ == 0);
}

TEST_CASE("integerSqrt returns floor sqrt and remainder")
{
    const IntSqrtResult exact = integerSqrt(49);
    REQUIRE(exact.p_ == 7);
    REQUIRE(exact.q_ == 0);

    const IntSqrtResult inexact = integerSqrt(50);
    REQUIRE(inexact.p_ == 7);
    REQUIRE(inexact.q_ == 1);
}

TEST_CASE("integerSqrt is exact for random 32-bit inputs")
{
    std::mt19937 rng{0xb760123c};
    std::uniform_int_distribution<std::int32_t> dist{ 0, std::numeric_limits<std::int32_t>::max()};

    for (int trial = 0; trial < 10; ++trial)
    {
        const int n = dist(rng);
        const IntSqrtResult result = integerSqrt(n);

        const std::int32_t p = result.p_;
        const std::int32_t q = result.q_;

        REQUIRE(p >= 0);
        REQUIRE(q >= 0);
        REQUIRE(p * p + q == n);
        REQUIRE(p * p <= n);
        REQUIRE((p + 1) * (p + 1) > n);
    }
}

TEST_CASE("integerSqrt is exact for all non-negative int32_t values")
{
    constexpr std::int32_t max_n = std::numeric_limits<std::int32_t>::max();

    // 16 passes with stride 16 and distinct offsets cover every value, while
    // each pass still walks from small to large.
    for (int offset = 0; offset < 16; ++offset)
    {
        for (std::int64_t n64 = offset; n64 <= max_n; n64 += 16)
        {
            const int n = static_cast<int>(n64);
            const IntSqrtResult result = integerSqrt(n);

            const std::int64_t p = result.p_;
            const std::int64_t q = result.q_;

            // Avoid Catch2 assertion overhead across ~2^31 iterations; use
            // int64 squares so (p+1)^2 does not overflow near INT_MAX.
            if (p < 0 || q < 0 || p * p + q != n64 || (p + 1) * (p + 1) <= n64)
            {
                FAIL("integerSqrt(" << n << ") => p=" << p << " q=" << q);
            }
        }
    }
    // Message that all non-negative int32_t values were exhaustively tested
    SUCCEED("All non-negative int32_t values were exhaustively tested");
}
