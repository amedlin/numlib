#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64) || defined(_M_ARM64EC))
#include <intrin.h>
#endif

static_assert(sizeof(int) == 4, "integerSqrt requires 32-bit int");

/// Result of an exact 32-bit integer square-root calculation.
///
/// For positive input n, p_ is floor(sqrt(n)) and q_ is the remainder such
/// that n == p_ * p_ + q_.
struct IntSqrtResult
{
    std::int32_t p_;
    std::int32_t q_;
};

/// Result of an exact 64-bit unsigned integer square-root calculation.
///
/// For input n, p_ is floor(sqrt(n)) and q_ is the remainder such that
/// n == p_ * p_ + q_ (with p_*p_ computed in 64-bit arithmetic; p_ fits in
/// 32 bits for every std::uint64_t n).
struct IntSqrt64Result
{
    std::uint64_t p_;
    std::uint64_t q_;
};

namespace detail
{

// Integer square root used during compile-time table generation and as a
// constexpr reference implementation.
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

// Wider reciprocal-sqrt table for 64-bit / Fixed paths: C ≈ 2^32 / sqrt(a).
// Same 1536 bins; 6144 bytes. With the 16-bit table, total stays under 16 KiB.
// Portable 64x64 -> 128 multiply. Fast paths use __int128 / MSVC intrinsics
// when available; schoolbook multiply otherwise (correct on all platforms).
constexpr void mul64to128(
    std::uint64_t a,
    std::uint64_t b,
    std::uint64_t& hi,
    std::uint64_t& lo) noexcept
{
    if (!std::is_constant_evaluated())
    {
#if defined(__SIZEOF_INT128__)
        const auto product =
            static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b);
        lo = static_cast<std::uint64_t>(product);
        hi = static_cast<std::uint64_t>(product >> 64);
        return;
#elif defined(_MSC_VER) && defined(_M_X64) && !defined(_M_ARM64EC)
        lo = _umul128(a, b, &hi);
        return;
#elif defined(_MSC_VER) && (defined(_M_ARM64) || defined(_M_ARM64EC))
        lo = a * b;
        hi = __umulh(a, b);
        return;
#endif
    }

    const std::uint64_t a_lo = a & 0xffffffffull;
    const std::uint64_t a_hi = a >> 32;
    const std::uint64_t b_lo = b & 0xffffffffull;
    const std::uint64_t b_hi = b >> 32;

    const std::uint64_t p0 = a_lo * b_lo;
    const std::uint64_t p1 = a_lo * b_hi;
    const std::uint64_t p2 = a_hi * b_lo;
    const std::uint64_t p3 = a_hi * b_hi;

    const std::uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffull) + (p2 & 0xffffffffull);
    lo = (p0 & 0xffffffffull) | (mid << 32);
    hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
}

[[nodiscard]]
constexpr std::uint64_t mul64Shift(std::uint64_t a, std::uint64_t b, unsigned shift) noexcept
{
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;
    mul64to128(a, b, hi, lo);
    if (shift >= 64u)
    {
        return hi >> (shift - 64u);
    }
    return (hi << (64u - shift)) | (lo >> shift);
}

// C = floor(2^37 / sqrt(d)) iff c^2 * d <= 2^74.
inline bool sqrtTable32EntryOk(std::uint32_t c, std::uint64_t d) noexcept
{
    const std::uint64_t c2 =
        static_cast<std::uint64_t>(c) * static_cast<std::uint64_t>(c);
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;
    mul64to128(c2, d, hi, lo);
    // 2^74 = 1024 << 64.
    return hi < 1024ull || (hi == 1024ull && lo == 0ull);
}

// Runtime-built (Clang rejects constexpr generation of this table). Kept as an
// inline const global — not a function-local static — so the hot path is a
// plain load with no per-call init guard.
[[nodiscard]]
inline std::array<std::uint32_t, SQRT_TABLE_SIZE> makeSqrtTable32() noexcept
{
    std::array<std::uint32_t, SQRT_TABLE_SIZE> built{};

    for (std::uint32_t i = 0; i < SQRT_TABLE_SIZE; ++i)
    {
        const std::uint64_t d = 2ull * (i + 512u) + 1ull;
        // Same centres as SQRT_TABLE: C ≈ 2^32 / sqrt(a) = 2^37 / sqrt(d).
        const std::uint64_t s = constexprSqrt((std::uint64_t{1} << 62) / d);
        std::uint32_t c = static_cast<std::uint32_t>(s << 6);

        while (c > 0u && !sqrtTable32EntryOk(c, d))
        {
            --c;
        }
        while (c < 0xffffffffu && sqrtTable32EntryOk(c + 1u, d))
        {
            ++c;
        }

        built[i] = c;
    }

    return built;
}

alignas(64)
inline const std::array<std::uint32_t, SQRT_TABLE_SIZE> SQRT_TABLE32 = makeSqrtTable32();

static_assert(sizeof(SQRT_TABLE32) == 6144);
static_assert(sizeof(SQRT_TABLE) + sizeof(SQRT_TABLE32) <= 16u * 1024u);

[[nodiscard]]
inline std::uint64_t polishFloorSqrt(std::uint64_t n, std::uint64_t p) noexcept
{
    if (p == 0u)
    {
        p = 1u;
    }
    if (p > 0xffffffffull)
    {
        p = 0xffffffffull;
    }

    std::uint64_t square = p * p;
    if (square > n)
    {
        do
        {
            --p;
            square -= 2u * p + 1u;
        } while (square > n);
    }
    else
    {
        for (;;)
        {
            const std::uint64_t delta = 2u * p + 1u;
            if (delta > n - square)
            {
                break;
            }
            square += delta;
            ++p;
        }
    }

    return p;
}

// LUT estimate using 32-bit C entries. Product is a full 64x64→128 multiply.
[[nodiscard]]
inline std::uint64_t lutSqrtEstimate32(std::uint64_t n) noexcept
{
    const unsigned e = 63u - static_cast<unsigned>(std::countl_zero(n));
    const unsigned e_even = e & ~1u;

    const std::uint64_t x_q30 = (e_even <= 30u)
        ? (n << (30u - e_even))
        : (n >> (e_even - 30u));
    const std::uint64_t y = x_q30 - (1ull << 30);
    const std::uint32_t k = static_cast<std::uint32_t>(y >> 21);
    const std::uint64_t c = SQRT_TABLE32[k];
    const std::uint64_t numerator =
        y + (static_cast<std::uint64_t>(k) << 21) + 0x80100000ull;

    // W=32 vs W=16 adds 16 shift bits relative to the 32-bit path (47 → 63).
    const unsigned shift = 63u - (e_even >> 1);
    return mul64Shift(numerator, c, shift);
}

// Exact floor(sqrt(n)) for uint64 via wide LUT + mul-based ±1 polish (no Newton).
[[nodiscard]]
inline std::uint64_t floorSqrtU64(std::uint64_t n) noexcept
{
    if (n <= 1u)
    {
        return n;
    }

    return polishFloorSqrt(n, lutSqrtEstimate32(n));
}

// Specialized Fixed raw sqrt: floor(sqrt(x << P)) for non-negative 32-bit x.
//
// Folds compile-time P into the normalization exponent instead of forming a
// general uint64 domain problem first. Polishes against n = x << P.
template <int P>
[[nodiscard]]
inline std::uint32_t floorSqrtFixedRaw(std::uint32_t x) noexcept
{
    static_assert(P > 0 && P < 32);

    if (x == 0u)
    {
        return 0u;
    }

    const std::uint64_t n = static_cast<std::uint64_t>(x) << P;
    const unsigned e_x = 31u - static_cast<unsigned>(std::countl_zero(x));
    const unsigned e_even = (e_x + static_cast<unsigned>(P)) & ~1u;

    // Mantissa of (x << P) in Q30 over [1, 4).
    const std::uint64_t x_q30 =
        static_cast<std::uint64_t>(x) << (30u - e_even + static_cast<unsigned>(P));
    const std::uint64_t y = x_q30 - (1ull << 30);
    const std::uint32_t k = static_cast<std::uint32_t>(y >> 21);
    const std::uint64_t c = SQRT_TABLE32[k];
    const std::uint64_t numerator =
        y + (static_cast<std::uint64_t>(k) << 21) + 0x80100000ull;

    const unsigned shift = 63u - (e_even >> 1);
    const std::uint64_t p = mul64Shift(numerator, c, shift);
    return static_cast<std::uint32_t>(polishFloorSqrt(n, p));
}

} // namespace detail


// Deleted primary: unsupported argument types fail at compile time.
// int uses an explicit specialization; uint64_t and Fixed use overloads
// (different return types; function templates cannot partially specialize).
template <typename T>
IntSqrtResult integerSqrt(T) noexcept = delete;

/// Calculates the exact floor square root and remainder of a signed 32-bit integer.
///
/// Positive input n returns {p, q}, where p = floor(sqrt(n)) and
/// n = p * p + q. Zero returns {0, 0}; negative input returns {0, input}.
template <>
[[nodiscard]]
inline IntSqrtResult integerSqrt<int>(int input) noexcept
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
        // step is 0 when p is already correct, and all-ones when p must
        // increase by one. p -= step then increments p, and delta & step
        // adds delta to the square.
        //
        const std::uint32_t delta = 2u * p + 1u;
        const std::uint32_t step = 0u - static_cast<std::uint32_t>((n - square) >= delta);

        p -= step;
        square += delta & step;
    }

    return
    {
        static_cast<int>(p),
        static_cast<int>(n - square)
    };
}

/// Exact floor square root and remainder for any std::uint64_t.
[[nodiscard]]
inline IntSqrt64Result integerSqrt(std::uint64_t input) noexcept
{
    if (input == 0u)
    {
        return { 0u, 0u };
    }

    const std::uint64_t p = detail::floorSqrtU64(input);
    const std::uint64_t square = p * p;
    return { p, input - square };
}
