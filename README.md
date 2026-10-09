# numlib — fixed-point arithmetic and integer square root for C++20

[![CI](https://github.com/amedlin/numlib/actions/workflows/ci.yml/badge.svg)](https://github.com/amedlin/numlib/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C.svg)](https://en.cppreference.com/w/cpp/20)

numlib is a small C++20 numerical library for binary fixed-point arithmetic
and fast, exact 32-bit integer square roots. It provides generic
`Fixed<P, Rep>` values, ready-to-use Q8.24, Q16.16, and Q24.8 types,
fixed-point math functions, and an integer square-root result with its exact
remainder.

## Features

- Signed `Fixed<P, Rep>` arithmetic with compile-time fractional precision
- 16-bit and 32-bit storage, plus 64-bit storage on compilers with `__int128`
- Q8.24 (`FixedF`), Q16.16 (`Fixed16`), and Q24.8 (`FixedI`) aliases
- Conversion, mixed-precision multiplication and division, square root,
  inverse square root, rounding, sine, cosine, and exponential approximation
- Exact `floor(sqrt(n))` for 32-bit signed integers, with remainder
- `constexpr` support for core fixed-point operations
- CMake targets for subdirectory and installed-package use
- Tested with MSVC, Clang, and GCC; requires an AVX2-capable target

Read the focused guides for [fixed-point arithmetic](docs/fixed-point.md) and
[integer square root](docs/integer-square-root.md).

## Quick example

```cpp
#include "fixedpt/fixed_types.h"
#include "int/isqrt.h"

Fixed16 price{12.5};
Fixed16 quantity{4};
Fixed16 total = price * quantity;

const IntSqrtResult root = integerSqrt(27);
// root.p_ == 5 and root.q_ == 2 because 27 == 5 * 5 + 2.
```

numlib is available under the [MIT License](LICENSE).

## What you need

- [CMake](https://cmake.org/) 3.20 or newer
- A C++20 compiler (MSVC, Clang, or GCC)
- [Cursor](https://cursor.com/) or [VS Code](https://code.visualstudio.com/)
- Extensions: **CMake Tools** and **C/C++** (recommended when you open the project)

## Install the toolchain

### Windows

1. Install [Visual Studio](https://visualstudio.microsoft.com/) 2022 or newer (including VS 2026).
2. In the installer, select the **Desktop development with C++** workload (MSVC + CMake).
3. Open this repo in Cursor/VS Code. CMake Tools will use the Visual Studio kit (no need to put `cl` or `cmake` on your PATH).

### macOS

1. Install the Xcode Command Line Tools:

   ```bash
   xcode-select --install
   ```

2. Install CMake if you do not have it (Homebrew):

   ```bash
   brew install cmake
   ```

### Linux

Install CMake and a C++20 compiler with your package manager.

Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y cmake g++ ninja-build
```

Fedora:

```bash
sudo dnf install -y cmake gcc-c++ ninja-build
```

## Open the project

1. Open the repo folder, or open `numlib.code-workspace`.
2. When prompted, install the recommended extensions (**CMake Tools**, **C/C++**).

## Configure and build (editor)

1. Select a CMake kit for your machine (MSVC, Clang, or GCC).
2. Configure with the **default** preset (CMake Tools status bar, or Command Palette → **CMake: Select Configure Preset**).
3. Build (status bar **Build**, or **CMake: Build**).
4. Set the launch/debug target to `test_isqrt`, `test_fixed`, or `perf`, then Run or Debug (F5). Launch configs **Debug test_isqrt**, **Debug test_fixed**, and **Debug perf** are provided.

## Configure and build (CLI)

From the repo root, on any OS:

```bash
# Debug
cmake --preset debug
cmake --build --preset debug

# Release
cmake --preset release
cmake --build --preset release
```

`default` is an untyped configure preset; prefer `debug` or `release`. Build presets pass the matching configuration for multi-config generators (for example Visual Studio).

Run the binaries from `build/`:

```bash
# Windows (multi-config)
build\Debug\test_isqrt.exe
build\Debug\test_fixed.exe
build\Release\perf.exe

# macOS / Linux (single-config; tree matches the preset you configured)
./build/test_isqrt
./build/test_fixed
./build/perf
```

Run `perf` in **Release** for meaningful timings. It uses a small `std::chrono` harness (no Catch2).

Unit tests use [Catch2](https://github.com/catchorg/Catch2), fetched by CMake at configure time (not checked into this repo). First configure needs network access. You can also run:

```bash
ctest --test-dir build --output-on-failure
```

## Use the library in your own code

`numlib` is a **static** library. Prefer consuming it with CMake:

```cmake
add_subdirectory(path/to/numlib)
target_link_libraries(your_target PRIVATE numlib::numlib)
```

You can also install numlib and consume its CMake package:

```bash
cmake --install build --config Release --prefix path/to/install
```

```cmake
find_package(numlib CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE numlib::numlib)
```

In your C++20 sources:

```cpp
#include "int/isqrt.h"
#include "fixedpt/fixed_types.h"
```

## Add implementation files

1. Add the `.cpp` under the right topic directory (for example `int/`).
2. List it in `target_sources` for the `numlib` target in `CMakeLists.txt`.

Headers hold the public API and anything needed for `constexpr`. Non-trivial runtime code goes in `.cpp` files. See [FORMAT.md](FORMAT.md) for naming and layout rules.
