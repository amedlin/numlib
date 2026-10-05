#pragma once

#include <array>
#include <bit>
#include <cstdint>

static_assert(sizeof(int) == 4, "integerSqrt requires 32-bit int");

struct IntSqrtResult
{
    std::int32_t p_;
    std::int32_t q_;
};

namespace detail
{

// Integer square root used ONLY during compile-time table generation.
//
// Returns floor(sqrt(n)).
constexpr std::uint64_t constexprSqrt(std::uint64_t n) noexcept
{
    std::uint64_t result = 0;
    std::uint64_t bit = std::uint64_t{1} << 62;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        const std::uint64_t candidate = result + bit;

        if (n >= candidate)
        {
            n -= candidate;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }

        bit >>= 2;
    }

    return result;
}


//
// We normalize:
//
//      n = x * 2^E
//
// where E is even and:
//
//      1 <= x < 4
//
// x is represented as Q30.
//
// We divide [1,4) into binary-aligned bins of width 1/512:
//
//      k = floor(x * 512)
//
// giving:
//
//      512 <= k <= 2047.
//
// The centre of bin k is:
//
//      a = (k + 1/2) / 512
//        = (2k + 1) / 1024.
//
// For each bin we store:
//
//      C[k] = floor(65536 / sqrt(a))
//
// Since:
//
//      sqrt(a) = sqrt((2k+1)/1024)
//
// this is:
//
//      C[k] = floor(2^21 / sqrt(2k+1))
//
// and therefore:
//
//      C[k] = floor(sqrt(2^42 / (2k+1))).
//
// The first 512 entries are unused, but retaining them means that
// the normalized mantissa indexes the table directly.
//
// SQRT_TABLE_SIZE * sizeof(uint16_t) = exactly 4096 bytes.
//
constexpr std::uint32_t SQRT_TABLE_SIZE = 2048;

constexpr std::array<std::uint16_t, SQRT_TABLE_SIZE> makeSqrtTable() noexcept
{
    std::array<std::uint16_t, SQRT_TABLE_SIZE> table{};

    constexpr std::uint64_t NUMERATOR =
        std::uint64_t{1} << 42;

    for (std::uint32_t k = 512; k < SQRT_TABLE_SIZE; ++k)
    {
        const std::uint64_t denominator =
            2ull * k + 1ull;

        table[k] =
            static_cast<std::uint16_t>(
                constexprSqrt(NUMERATOR / denominator));
    }

    return table;
}


alignas(64)
inline constexpr auto SQRT_TABLE = makeSqrtTable();

static_assert(sizeof(SQRT_TABLE) == SQRT_TABLE_SIZE * sizeof(std::uint16_t));

} // namespace detail


[[nodiscard]]
inline IntSqrtResult integerSqrt(int input) noexcept
{
    //
    // This also handles zero:
    //
    //     input < 0 -> { 0, input }
    //     input = 0 -> { 0, 0 }
    //
    if (input <= 0)
    {
        return { 0, input };
    }

    const std::uint32_t n =
        static_cast<std::uint32_t>(input);

    //
    // e = floor(log2(n))
    //
    const unsigned e =
        31u - std::countl_zero(n);

    //
    // Force the exponent even:
    //
    //     E = 0, 2, 4, ... 30
    //
    const unsigned e_even =
        e & ~1u;

    //
    // Normalize n into Q30:
    //
    //     x = n / 2^E
    //
    // so:
    //
    //     1 <= x < 4
    //
    // and:
    //
    //     x_q30 = x * 2^30.
    //
    const std::uint64_t x_q30 =
        static_cast<std::uint64_t>(n)
        << (30u - e_even);

    //
    // Since each bin has width 1/512:
    //
    //     k = floor(x * 512)
    //
    // x_q30 contains 30 fractional bits, therefore:
    //
    //     k = x_q30 >> (30 - 9)
    //       = x_q30 >> 21.
    //
    // Range is 512..2047.
    //
    const std::uint32_t k =
        static_cast<std::uint32_t>(x_q30 >> 21);

    //
    // C ~= 2^16 / sqrt(a)
    //
    const std::uint64_t c =
        detail::SQRT_TABLE[k];

    //
    // Bin centre:
    //
    //     a = (2k + 1) / 1024.
    //
    // In Q30:
    //
    //     a_q30
    //       = (2k+1) * 2^30 / 2^10
    //       = (2k+1) << 20.
    //
    const std::uint64_t a_q30 =
        (2ull * k + 1ull) << 20;

    //
    // Tangent approximation to sqrt(x) at a:
    //
    //                  x + a
    //     sqrt(x) ~= ----------
    //                 2 sqrt(a)
    //
    // Since:
    //
    //     C ~= 2^16 / sqrt(a)
    //
    // and x+a is Q30, the product:
    //
    //     (x_q30 + a_q30) * C
    //
    // represents:
    //
    //     (x+a) / sqrt(a)
    //
    // with 46 fractional bits.
    //
    // The additional division by 2 gives 47 fractional bits.
    //
    // Finally:
    //
    //     sqrt(n) = sqrt(x) * 2^(E/2)
    //
    // so combine both scalings into one shift.
    //
    const unsigned shift =
        47u - (e_even >> 1);

    const std::uint32_t r =
        static_cast<std::uint32_t>(
            ((x_q30 + a_q30) * c) >> shift);

    //
    // The approximation guarantees that r differs from
    // floor(sqrt(n)) by at most one.
    //
    std::uint32_t p = r;
    std::uint32_t square = p * p;

    //
    // Exactification.
    //
    // Only ONE of these corrections can ever be necessary.
    //
    if (square > n)
    {
        //
        // r was one too high.
        //
        // After decrement:
        //
        //     old_p^2 - new_p^2
        //       = 2*new_p + 1
        //
        --p;

        square -= 2u * p + 1u;
    }
    else
    {
        //
        // Test whether p+1 is still <= sqrt(n).
        //
        // No multiplication is necessary because:
        //
        //     (p+1)^2 = p^2 + 2p + 1.
        //
        const std::uint32_t delta =
            2u * p + 1u;

        if (n - square >= delta)
        {
            square += delta;
            ++p;
        }
    }

    //
    // Exact result:
    //
    //     n = p*p + q
    //
    return
    {
        static_cast<int>(p),
        static_cast<int>(n - square)
    };
}
