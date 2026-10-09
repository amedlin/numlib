#pragma once

#include "fixedpt/fixed.h"

/// Signed 32-bit Q16.16 fixed-point number with 16 fractional bits.
using Fixed16 = Fixed<16>;

/// Signed 32-bit Q8.24 fixed-point number with 24 fractional bits.
using FixedF = Fixed<24>;

/// Signed 32-bit Q24.8 fixed-point number with 8 fractional bits.
using FixedI = Fixed<8>;

/// Approximates e^value for a Q16.16 value with the quinapalus bit algorithm.
[[nodiscard]] Fixed16 expApprox(Fixed16 value) noexcept;
