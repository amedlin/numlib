#include "fixedpt/fixed_types.h"

namespace
{

// Approximate exp for Q16.16, non-negative inputs.
// Sourced from http://www.quinapalus.com/efunc.html
[[nodiscard]]
constexpr std::int32_t expApproxFixed16(std::int32_t x) noexcept
{
    std::int32_t y = 0x00010000;

    const auto try_shift = [&](std::int32_t threshold, int shift) constexpr noexcept
    {
        const std::int32_t trial = x - threshold;
        if (trial >= 0)
        {
            x = trial;
            y <<= shift;
        }
    };

    const auto try_add = [&](std::int32_t threshold, int shift) constexpr noexcept
    {
        const std::int32_t trial = x - threshold;
        if (trial >= 0)
        {
            x = trial;
            y += y >> shift;
        }
    };

    try_shift(0x58b91, 8);
    try_shift(0x2c5c8, 4);
    try_shift(0x162e4, 2);
    try_shift(0x0b172, 1);
    try_add(0x067cd, 1);
    try_add(0x03920, 2);
    try_add(0x01e27, 3);
    try_add(0x00f85, 4);
    try_add(0x007e1, 5);
    try_add(0x003f8, 6);
    try_add(0x001fe, 7);

    for (int bit = 8; bit <= 16; ++bit)
    {
        if ((x & (1 << (16 - bit))) != 0)
        {
            y += y >> bit;
        }
    }

    return y;
}

} // namespace


Fixed16 expApprox(Fixed16 value) noexcept
{
    if (value >= 0)
    {
        return Fixed16::fromRaw(expApproxFixed16(value.getRawValue()));
    }

    return Fixed16::fromRaw(expApproxFixed16((-value).getRawValue())).inverseApprox();
}
