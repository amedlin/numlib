# Fixed-point arithmetic

numlib provides signed binary fixed-point numbers for C++20. `Fixed<P, Rep>`
uses `P` fractional bits in the signed integral representation `Rep`. The
default representation is `std::int32_t`, with `32 - P` integral bits
including the sign bit. This default layout is commonly written as
`Q(32-P).P`.

## Ready-to-use Q formats

Include `fixedpt/fixed_types.h` to use these aliases:

- `FixedF` is `Fixed<24>`, a Q8.24 fixed-point number.
- `Fixed16` is `Fixed<16>`, a Q16.16 fixed-point number.
- `FixedI` is `Fixed<8>`, a Q24.8 fixed-point number.

```cpp
#include "fixedpt/fixed_types.h"

Fixed16 x{3.25};
Fixed16 y{2};
Fixed16 product = x * y;

float result = product.getFloat(); // 6.5
```

Use `Fixed<P, Rep>` directly when another division between integral and
fractional bits or another storage width is required:

```cpp
#include "fixedpt/fixed.h"

using Q12_20 = Fixed<20>;
Q12_20 value{1, 3};

using Q8_8 = Fixed<8, std::int16_t>;
```

The library supports `std::int16_t` and `std::int32_t` representations. It
also supports `std::int64_t` when the compiler provides `__int128` for
intermediate calculations.

## Operations

The fixed-point API supports:

- arithmetic and comparisons;
- conversion between fixed-point precisions with `convert`;
- mixed-precision `mulAs` and `divAs` operations;
- fused-style fixed-point multiply-add with `mulAdd`;
- `sqrt`, `invSqrt`, `inverse`, `square`, and approximate variants;
- `round`, `floor`, `ceil`, and power-of-two scaling;
- `sin`, `cos`, and combined `sinCos`;
- conversion to and from raw signed integer representations.

`fixedpt/fixed_types.h` also declares `expApprox` for Q16.16 values.

## Range and overflow

Operations use assertions to detect many invalid inputs and out-of-range
results in builds where assertions are enabled. The type does not provide
saturating arithmetic. Before an operation, make sure that its result fits
the selected Q format.

Raw-value access is available through `getRawValue`, `setRawValue`, and
`fromRaw`. Use these functions when interoperating with binary protocols,
hardware registers, or other fixed-point implementations.
