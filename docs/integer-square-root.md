# Exact integer square root

`integerSqrt` calculates the floor of the square root of a signed 32-bit
integer and returns the exact remainder. The implementation uses an integer
lookup table and integer arithmetic; it does not call the floating-point
`sqrt` function.

```cpp
#include "int/isqrt.h"

const IntSqrtResult result = integerSqrt(27);

// result.p_ == 5
// result.q_ == 2
// 27 == result.p_ * result.p_ + result.q_
```

For a positive input `n`, the result satisfies:

```text
p = floor(sqrt(n))
n = p * p + q
0 <= q < 2 * p + 1
```

Zero returns `{0, 0}`. A negative input returns `{0, input}` because no
non-negative integer square root exists.

## Requirements

The public function accepts `int` and requires a platform with 32-bit `int`.
The current CMake target also requires AVX2 when it compiles consumers of this
inline header.

## Use cases

An exact integer square root is useful for integer geometry, number theory,
image processing, embedded calculations, and algorithms that need both the
root and the residual value without floating-point rounding.
