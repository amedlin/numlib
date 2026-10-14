#pragma once

#include <array>
#include <cstdint>

// Table-driven sine on a binary phase.
//
// The angle is a "phase": an unsigned 64-bit fraction of one turn (Q0.64
// turns). Whole turns vanish in the 64-bit wrap, so range reduction is
// free. The top two bits select the quadrant, the next 8 bits select a
// table node, and the remaining bits are the offset inside the bin. A
// cubic Taylor step from the node gives the result in Q2.30.
//
// Table size: 257 nodes x 16 bytes = 4112 bytes. Max error: 0.80 LSB (Q30).
// sin(0), sin(pi/2) = +1, sin(pi) = 0, and sin(3pi/2) = -1 are exact.

namespace detail
{

inline constexpr double SIN_PI = 3.14159265358979323846264338327950288;

// Taylor series; accurate to double precision for |x| <= pi/2.
[[nodiscard]]
constexpr double constexprSin(double x) noexcept
{
    const double x2 = x * x;
    double term = x;
    double sum = x;
    for (int n = 1; n < 20; ++n)
    {
        term *= -x2 / static_cast<double>((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}

[[nodiscard]]
constexpr double constexprCos(double x) noexcept
{
    const double x2 = x * x;
    double term = 1.0;
    double sum = 1.0;
    for (int n = 1; n < 20; ++n)
    {
        term *= -x2 / static_cast<double>((2 * n - 1) * (2 * n));
        sum += term;
    }
    return sum;
}

// Quadrant [0, pi/2) is split into 2^SIN_BITS bins. Node k sits at the left
// edge of bin k (angle k * SIN_STEP); node 2^SIN_BITS is the right edge, pi/2.
constexpr int SIN_BITS = 8;
constexpr std::uint32_t SIN_NODE_COUNT = (std::uint32_t{1} << SIN_BITS) + 1u;
constexpr double SIN_STEP = (SIN_PI / 2.0) / static_cast<double>(std::uint32_t{1} << SIN_BITS);

// Scaled Taylor coefficients at a node, with angle x = k * h, h = SIN_STEP:
//
//     s_  = sin(x)              Q31  (node 256 is exactly 2^31)
//     t1_ = h   * cos(x)        Q38
//     t2_ = h^2 * sin(x) / 2    Q46
//     t3_ = h^3 * cos(x) / 6    Q55
//
// With u the Q32 bin fraction, sin(x + u*h) = s_ + u*(t1_ - u*(t2_ + u*t3_))
// after the matching shifts. Each coefficient fits in 32 bits.
struct SinNode
{
    std::uint32_t s_;
    std::uint32_t t1_;
    std::uint32_t t2_;
    std::uint32_t t3_;
};

[[nodiscard]]
constexpr std::uint32_t roundToU32(double value) noexcept
{
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value + 0.5));
}

[[nodiscard]]
constexpr std::array<SinNode, SIN_NODE_COUNT> makeSinTable() noexcept
{
    std::array<SinNode, SIN_NODE_COUNT> table{};

    constexpr double H = SIN_STEP;
    constexpr double SCALE_S = static_cast<double>(std::uint64_t{1} << 31);
    constexpr double SCALE_T1 = static_cast<double>(std::uint64_t{1} << 38);
    constexpr double SCALE_T2 = static_cast<double>(std::uint64_t{1} << 46);
    constexpr double SCALE_T3 = static_cast<double>(std::uint64_t{1} << 55);

    for (std::uint32_t k = 0; k < SIN_NODE_COUNT; ++k)
    {
        const double x = static_cast<double>(k) * H;
        const double s = constexprSin(x);
        const double c = constexprCos(x);

        table[k].s_ = roundToU32(s * SCALE_S);
        table[k].t1_ = roundToU32(H * c * SCALE_T1);
        table[k].t2_ = roundToU32(H * H * s * 0.5 * SCALE_T2);
        table[k].t3_ = roundToU32(H * H * H * c * (1.0 / 6.0) * SCALE_T3);
    }

    // Pin the quadrant edges so sin(0) = 0 and sin(pi/2) = 1 are exact.
    table[0].s_ = 0u;
    table[0].t2_ = 0u;
    table[SIN_NODE_COUNT - 1u].s_ = std::uint32_t{1} << 31;
    table[SIN_NODE_COUNT - 1u].t1_ = 0u;
    table[SIN_NODE_COUNT - 1u].t3_ = 0u;

    return table;
}

alignas(64)
inline constexpr auto SIN_TABLE = makeSinTable();

static_assert(sizeof(SIN_TABLE) == 4112);
static_assert(SIN_TABLE[0].s_ == 0u);
static_assert(SIN_TABLE[SIN_NODE_COUNT - 1u].s_ == (std::uint32_t{1} << 31));

// floor(2^64 / (2 pi)): one radian in Q0.64 turns.
inline constexpr std::uint64_t INV_TWO_PI_Q64 = 0x28BE60DB9391054Aull;

// round(2^(64-P) / (2 pi)): multiplies a Q(P) radian raw value into Q0.64 turns.
template <int P>
inline constexpr std::uint64_t PHASE_SCALE = (INV_TWO_PI_Q64 + (std::uint64_t{1} << (P - 1))) >> P;

// sin(2 pi * phase / 2^64) in Q2.30.
[[nodiscard]]
constexpr std::int32_t sinPhaseQ30(std::uint64_t phase) noexcept
{
    //
    // Quadrant in the top two bits. The rest, shifted up by one, is the
    // 63-bit offset into the quadrant: 0 <= r < 2^63.
    //
    const unsigned quadrant = static_cast<unsigned>(phase >> 62);
    const std::uint64_t r = (phase << 2) >> 1;

    //
    // Odd quadrants run backwards: sin(pi - x) = sin(x). The mirror
    // 2^63 - r maps the quarter turn to node 256, so +1 is exact.
    //
    const std::uint64_t pos = (quadrant & 1u) != 0u ? (std::uint64_t{1} << 63) - r : r;

    //
    // Node index (0..256) and Q32 fraction inside the bin.
    //
    const std::uint32_t k = static_cast<std::uint32_t>(pos >> (63 - SIN_BITS));
    const std::int64_t u = static_cast<std::int64_t>((pos << (SIN_BITS + 1)) >> 32);

    const SinNode& node = SIN_TABLE[k];

    //
    // Horner: s + u*(t1 - u*(t2 + u*t3)). Every product is (< 2^31) * (< 2^32)
    // and fits in a signed 64-bit value.
    //
    std::int64_t acc = static_cast<std::int64_t>(node.t3_);
    acc = ((acc * u) >> 41) + static_cast<std::int64_t>(node.t2_);
    acc = static_cast<std::int64_t>(node.t1_) - ((acc * u) >> 40);
    const std::int64_t q31 = static_cast<std::int64_t>(node.s_) + ((acc * u) >> 39);

    //
    // Round Q31 to Q30, then apply the sign of the half turn.
    //
    const auto magnitude = static_cast<std::int32_t>((q31 + 1) >> 1);
    return (quadrant & 2u) != 0u ? -magnitude : magnitude;
}

} // namespace detail
