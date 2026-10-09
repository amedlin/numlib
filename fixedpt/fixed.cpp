#include "fixedpt/fixed_types.h"

namespace
{

// Approximate exp for Q16.16, non-negative inputs.
// Sourced from http://www.quinapalus.com/efunc.html
std::int32_t expApproxFixed16(std::int32_t x) noexcept
{
    std::int32_t y = 0x00010000;
    std::int32_t t = x - 0x58b91;
    if (t >= 0)
    {
        x = t;
        y <<= 8;
    }
    t = x - 0x2c5c8;
    if (t >= 0)
    {
        x = t;
        y <<= 4;
    }
    t = x - 0x162e4;
    if (t >= 0)
    {
        x = t;
        y <<= 2;
    }
    t = x - 0x0b172;
    if (t >= 0)
    {
        x = t;
        y <<= 1;
    }
    t = x - 0x067cd;
    if (t >= 0)
    {
        x = t;
        y += y >> 1;
    }
    t = x - 0x03920;
    if (t >= 0)
    {
        x = t;
        y += y >> 2;
    }
    t = x - 0x01e27;
    if (t >= 0)
    {
        x = t;
        y += y >> 3;
    }
    t = x - 0x00f85;
    if (t >= 0)
    {
        x = t;
        y += y >> 4;
    }
    t = x - 0x007e1;
    if (t >= 0)
    {
        x = t;
        y += y >> 5;
    }
    t = x - 0x003f8;
    if (t >= 0)
    {
        x = t;
        y += y >> 6;
    }
    t = x - 0x001fe;
    if (t >= 0)
    {
        x = t;
        y += y >> 7;
    }

    if (x & 0x100)
    {
        y += y >> 8;
    }
    if (x & 0x080)
    {
        y += y >> 9;
    }
    if (x & 0x040)
    {
        y += y >> 10;
    }
    if (x & 0x020)
    {
        y += y >> 11;
    }
    if (x & 0x010)
    {
        y += y >> 12;
    }
    if (x & 0x008)
    {
        y += y >> 13;
    }
    if (x & 0x004)
    {
        y += y >> 14;
    }
    if (x & 0x002)
    {
        y += y >> 15;
    }
    if (x & 0x001)
    {
        y += y >> 16;
    }
    return y;
}

} // namespace


Fixed16 Fixed16::expApprox() const noexcept
{
    Fixed16 result;
    if ((*this) >= 0)
    {
        result.x_ = expApproxFixed16(x_);
        return result;
    }

    result.x_ = expApproxFixed16((-(*this)).x_);
    return result.inverseApprox();
}
