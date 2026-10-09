#pragma once

#include "fixedpt/fixed.h"

// Q16.16 fixed-point number.
using Fixed16 = Fixed<16>;

// Q8.24 fixed-point number.
using FixedF = Fixed<24>;

// Q24.8 fixed-point number.
using FixedI = Fixed<8>;

// Approximate exp for Q16.16 (quinapalus bit algorithm).
[[nodiscard]] Fixed16 expApprox(Fixed16 value) noexcept;
