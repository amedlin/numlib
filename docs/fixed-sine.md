# Fast fixed-point sine

`Fixed<P, Rep>::sin()` is a table-driven, integer-only sine for 16-bit and
32-bit representations. It returns `FixedN` (`Fixed<30>`, Q2.30), so the
result holds `-1` and `+1` exactly. It does not use floating point, division,
or `fmod`, and it is `constexpr`.

```cpp
#include "fixedpt/fixed_types.h"

const Fixed16 angle{1.5707963f};     // radians
const FixedN s = angle.sin();        // ~1.0 in Q2.30

const FixedN quarter = sinTurns(0x40000000u); // exactly 1.0
```

The previous CORDIC implementation is still available as `sinCordic()` and
returns the input type.

## Why Q2.30 (`FixedN`)

A sine result is always inside `[-1, +1]`. A Q1.31 format cannot hold `+1`
(its range is `[-1, 1)`), so Q2.30 is used. One bit of range is spent so that
both end points are exact; the remaining 30 fractional bits give a resolution
of about `9.3e-10`.

## Phase representation

The core works on a *phase*: the angle in turns as an unsigned Q0.64 fraction.

```text
phase = angle / (2 pi) * 2^64   (mod 2^64)
```

Whole turns disappear in the 64-bit wrap, so range reduction is free. The
phase is split as:

| Bits | Meaning |
| --- | --- |
| 63..62 | quadrant (0..3) |
| 61..54 | table node `k` inside the quadrant (0..255) |
| 53..0 | offset inside the bin |

Odd quadrants are mirrored (`sin(pi - x) = sin(x)`), and the upper half turn
negates the result. The mirror maps a full quarter turn to node 256, so
`sin(pi/2) = +1` is exact.

### Radians

`Fixed<P, Rep>::sin()` converts the raw Q(P) radian value to a phase with one
wrapping 64-bit multiply:

```text
phase = uint64(int64(raw)) * PHASE_SCALE<P>
PHASE_SCALE<P> = round(2^(64 - P) / (2 pi))
```

The constant rounding costs at most 0.4 Q30 LSB over the full 32-bit raw range.

### Turns

`sinTurns(std::uint32_t turns_q32)` takes the angle directly in turns as a
Q0.32 fraction (`0x40000000` is a quarter turn). It skips the multiply and is
exact at every quarter turn. These identities hold on the raw results:

```text
sinTurns(-t)        == -sinTurns(t)
sinTurns(t + half)  == -sinTurns(t)
sinTurns(half - t)  ==  sinTurns(t)
```

## Table

The first quadrant is split into 256 bins of width `h = pi / 512`. Node `k`
sits at the left edge of bin `k`, and node 256 is `pi / 2`. Each node stores
four scaled Taylor coefficients, all of which fit in 32 bits:

| Field | Value | Format |
| --- | --- | --- |
| `s_` | `sin(x)` | Q31 (node 256 is exactly `2^31`) |
| `t1_` | `h cos(x)` | Q38 |
| `t2_` | `h^2 sin(x) / 2` | Q46 |
| `t3_` | `h^3 cos(x) / 6` | Q55 |

With `u` the Q32 fraction inside the bin, the result is evaluated by Horner:

```text
sin(x + u h) ~= s + u (t1 - u (t2 + u t3))
```

followed by a rounding shift from Q31 to Q30. The table has 257 nodes of 16
bytes (`4112` bytes), is 64-byte aligned, and is built at compile time from a
Taylor series. Together with the square-root tables the lookup tables total
13,328 bytes; a `static_assert` in `fixedpt/fixed.h` keeps them under 16 KiB.

## Accuracy

- Cubic truncation error: `h^4 / 24 = 0.063` Q30 LSB.
- Measured maximum error of `sinTurns` over every turn in the first quadrant
  (exhaustive, `2^30` inputs): **0.80 Q30 LSB**. The other quadrants are exact
  mirrors.
- Measured maximum error of `Fixed16::sin()` over random full-range inputs:
  **0.80 Q30 LSB**.
- The result never leaves `[-2^30, +2^30]`.

## Performance

The hot path is one 64-bit multiply (radians only), one 16-byte load, and
three dependent 64-bit multiplies. See [perf.md](perf.md) for timings against
`std::sin` and CORDIC.
