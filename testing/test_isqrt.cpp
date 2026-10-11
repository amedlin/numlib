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

TEST_CASE("integerSqrt is exact for all non-negative int32_t values", "[.exhaustive]")
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

namespace
{

void requireExactU64(std::uint64_t n)
{
    const IntSqrt64Result result = integerSqrt(n);
    const std::uint64_t p = result.p_;
    const std::uint64_t q = result.q_;

    REQUIRE(p <= 0xffffffffull);
    REQUIRE(p * p + q == n);
    REQUIRE(p * p <= n);
    if (p < 0xffffffffull)
    {
        REQUIRE((p + 1u) * (p + 1u) > n);
    }
    REQUIRE(p == detail::constexprSqrt(n));
}

} // namespace

TEST_CASE("integerSqrt uint64 handles edges")
{
    requireExactU64(0u);
    requireExactU64(1u);
    requireExactU64(2u);
    requireExactU64(3u);
    requireExactU64(4u);

    constexpr std::uint64_t max_root = 0xffffffffull;
    requireExactU64(max_root * max_root);
    requireExactU64(std::numeric_limits<std::uint64_t>::max());

    for (unsigned k = 0; k < 64; ++k)
    {
        requireExactU64(1ull << k);
        if (k > 0)
        {
            requireExactU64((1ull << k) - 1u);
        }
        if (k < 63)
        {
            requireExactU64((1ull << k) + 1u);
        }
    }

    // Near perfect squares across the range.
    for (std::uint64_t root : {std::uint64_t{2}, std::uint64_t{255}, std::uint64_t{256},
                               std::uint64_t{65535}, std::uint64_t{65536},
                               std::uint64_t{0x100000}, max_root - 1u, max_root})
    {
        const std::uint64_t square = root * root;
        requireExactU64(square);
        if (square > 0)
        {
            requireExactU64(square - 1u);
        }
        if (square < std::numeric_limits<std::uint64_t>::max())
        {
            requireExactU64(square + 1u);
        }
    }
}

TEST_CASE("integerSqrt uint64 is exact for random and exponent-decade samples")
{
    std::mt19937_64 rng{0xC0FFEEULL};
    std::uniform_int_distribution<std::uint64_t> dist{
        0u,
        std::numeric_limits<std::uint64_t>::max()};

    for (int trial = 0; trial < 10'000; ++trial)
    {
        requireExactU64(dist(rng));
    }

    // Dense coverage in each exponent decade: values with floor(log2) == e.
    for (unsigned e = 0; e < 64; ++e)
    {
        const std::uint64_t base = 1ull << e;
        const std::uint64_t top =
            (e == 63u) ? std::numeric_limits<std::uint64_t>::max()
                       : ((1ull << (e + 1u)) - 1u);

        requireExactU64(base);
        requireExactU64(top);
        requireExactU64(base + (top - base) / 3u);
        requireExactU64(base + (2u * (top - base)) / 3u);

        // Mantissa bin boundaries after normalizing into [1, 4).
        for (unsigned bin = 0; bin < 8; ++bin)
        {
            const std::uint64_t span = top - base;
            requireExactU64(base + (span * bin) / 7u);
        }
    }
}
