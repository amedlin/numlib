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


// Normalized x is in [1,4).
//
// Divide this into binary-aligned bins of width 1/512:
//
//     k = floor(x * 512)
//
// so:
//
//     512 <= k <= 2047.
//
// Rather than storing entries [0,2047], store only the useful
// entries:
//
//     table[k - 512]
//
// giving exactly 1536 entries.
//
// Each entry contains:
//
//     C[k] = floor(65536 / sqrt(a))
//
// where:
//
//     a = (k + 1/2) / 512
//       = (2k + 1) / 1024.
//
// Therefore:
//
//     C[k]
//       = floor(2^21 / sqrt(2k+1))
//       = floor(sqrt(2^42 / (2k+1))).
//
constexpr std::uint32_t SQRT_TABLE_SIZE = 1536;

constexpr std::array<std::uint16_t, SQRT_TABLE_SIZE> makeSqrtTable() noexcept
{
    std::array<std::uint16_t, SQRT_TABLE_SIZE> table{};

    constexpr std::uint64_t NUMERATOR = std::uint64_t{1} << 42;

    for (std::uint32_t i = 0; i < SQRT_TABLE_SIZE; ++i)
    {
        const std::uint32_t k = i + 512;
        const std::uint64_t denominator = 2ull * k + 1ull;
        table[i] = static_cast<std::uint16_t>(
            constexprSqrt(NUMERATOR / denominator));
    }

    return table;
}


alignas(64)
inline constexpr auto SQRT_TABLE = makeSqrtTable();

static_assert(sizeof(SQRT_TABLE) == 3072);

} // namespace detail


[[nodiscard]]
inline IntSqrtResult integerSqrt(int input) noexcept
{
    if (input <= 0)
    {
        return { 0, input };
    }

    const std::uint32_t n = static_cast<std::uint32_t>(input);

    //
    // e = floor(log2(n))
    //
    const unsigned e = 31u - std::countl_zero(n);

    //
    // Force exponent even.
    //
    const unsigned e_even = e & ~1u;

    //
    // Normalize into Q30:
    //
    //     x = n / 2^E
    //
    // where:
    //
    //     1 <= x < 4.
    //
    const std::uint64_t x_q30 = static_cast<std::uint64_t>(n) << (30u - e_even);

    //
    // Remove the implicit 1.0.
    //
    // y represents x - 1 in Q30:
    //
    //     0 <= y < 3 * 2^30
    //
    const std::uint64_t y = x_q30 - (1ull << 30);

    //
    // Bin number / zero-based LUT index:
    //
    // 1536 bins across [1,4), each of width 1/512.
    //
    //     k = floor(y * 512) = floor((x - 1) * 512)
    //
    // so:
    //
    //     0 <= k <= 1535
    //
    // and the corresponding absolute bin used in the table
    // formulas is K = k + 512 (with 512 <= K <= 2047).
    //
    const std::uint32_t k = static_cast<std::uint32_t>(y >> 21);

    //
    // C ~= 2^16 / sqrt(a), looked up by zero-based index k.
    //
    const std::uint64_t c = detail::SQRT_TABLE[k];

    //
    // Centre of absolute bin K = k + 512:
    //
    //     a = (2K + 1) / 1024
    //       = (2k + 1025) / 1024
    //
    // The tangent numerator is x + a in Q30.  Expanding and
    // rewriting in terms of y and k gives the equivalent form:
    //
    //     x_q30 + a_q30
    //       = y + (k << 21) + 0x80100000
    //
    // where 0x80100000 = 2^31 + 2^20.
    //
    const std::uint64_t numerator = y + (static_cast<std::uint64_t>(k) << 21) + 0x80100000ull;

    //
    // Tangent approximation:
    //
    //                 x + a
    //     sqrt(x) ~= ---------
    //                2 sqrt(a)
    //
    // C ~= 2^16 / sqrt(a).
    //
    const unsigned shift = 47u - (e_even >> 1);
    std::uint32_t p = static_cast<std::uint32_t>((numerator * c) >> shift);

    //
    // The LUT approximation is within one integer of the exact
    // floor(sqrt(n)), so only one correction can be required.
    //
    std::uint32_t square = p * p;

    if (square > n)
    {
        //
        // p is one too large.
        //
        --p;

        square -= 2u * p + 1u;
    }
    else
    {
        //
        // Test p+1 without another multiplication:
        //
        //     (p+1)^2 = p^2 + 2p + 1.
        //
        const std::uint32_t delta = 2u * p + 1u;

        if (n - square >= delta)
        {
            square += delta;
            ++p;
        }
    }

    return
    {
        static_cast<int>(p),
        static_cast<int>(n - square)
    };
}
