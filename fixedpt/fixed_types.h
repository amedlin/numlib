#pragma once

#include "fixedpt/fixed.h"

/// Signed 32-bit Q16.16 fixed-point number with 16 fractional bits.
using Fixed16 = Fixed<16>;

/// Signed 32-bit Q8.24 fixed-point number with 24 fractional bits (prioritizes fractional precision over integer precision)
using FixedF = Fixed<24>;

/// Signed 32-bit Q24.8 fixed-point number with 8 fractional bits (prioritizes integer precision over fractional precision)
using FixedI = Fixed<8>;

/// Signed 32-bit Q2.30 normalized fixed-point number with 30 fractional bits.
/// Holds the closed interval [-1, +1] with both end points exact (range is
/// [-2, 2)). Used for results such as sine that never leave [-1, +1].
using FixedN = Fixed<30>;

/// Approximates e^value for a Q16.16 value with the quinapalus bit algorithm.
[[nodiscard]] Fixed16 expApprox(Fixed16 value) noexcept;
