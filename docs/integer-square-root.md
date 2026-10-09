# Exact integer square root

`integerSqrt` is a deleted primary template with an `int` specialization and a
`std::uint64_t` overload (different return types). Each returns the floor
square root and the exact remainder. The implementation uses an integer lookup
table and integer arithmetic; it does not call the floating-point `sqrt`
function.

```cpp
#include "int/isqrt.h"

const IntSqrtResult result32 = integerSqrt(27);
// result32.p_ == 5, result32.q_ == 2, 27 == 5*5 + 2

const IntSqrt64Result result64 = integerSqrt(std::uint64_t{27});
// result64.p_ == 5, result64.q_ == 2
```

For a positive / non-zero input `n`, the result satisfies:

```text
p = floor(sqrt(n))
n = p * p + q
0 <= q < 2 * p + 1
```

For `int`: zero returns `{0, 0}`; a negative input returns `{0, input}`.
For `std::uint64_t`: zero returns `{0, 0}`.

## Range reduction and LUT size

Both widths normalize `n` into a mantissa `m ∈ [1, 4)` by forcing an even
`floor(log2(n))`, then approximate `sqrt(m)` from a table and scale by
`2^(E/2)`. Table size therefore depends only on how finely `[1, 4)` is sampled,
not on the input bit width.

Two tables share the same **1536** bins of width `1/512` over `[1, 4)`,
each aligned to 64 bytes:

- `SQRT_TABLE`: `uint16_t` entries (`C ≈ 2^16 / sqrt(a)`), **3072 bytes**,
  used by the `int` specialization (exact after a single ±1 correction).
- `SQRT_TABLE32`: `uint32_t` entries (`C ≈ 2^32 / sqrt(a)`), **6144 bytes**,
  used by the `uint64_t` overload and the Fixed specialized path (mul-based
  polish; no Newton division).

A `static_assert` keeps the combined LUT size under a conservative **16 KiB**
L1 budget (9216 bytes total).

## Fixed-point overload

`fixedpt/fixed.h` provides:

```cpp
template <int P>
Fixed<P, std::int32_t> integerSqrt(Fixed<P, std::int32_t> value);
```

This covers `Fixed16`, `FixedF`, and `FixedI`. It folds fractional precision
`P` into the normalization exponent (specialized path, not a call through
`integerSqrt(uint64_t)`), and returns a value of the same Fixed type. For
these formats, `Fixed::sqrt()` delegates to this overload at runtime.

## Requirements

The `int` specialization requires a platform with 32-bit `int`. On x86/x64
hosts, CMake may enable AVX2 for consumers of this inline header when the
compiler supports it; ARM and other ISAs skip that flag.

## Use cases

An exact integer square root is useful for integer geometry, number theory,
image processing, embedded calculations, fixed-point math, and algorithms that
need both the root and the residual value without floating-point rounding.
