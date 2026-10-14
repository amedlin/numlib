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
- Q8.24 (`FixedF`), Q16.16 (`Fixed16`), Q24.8 (`FixedI`), and normalized
  Q2.30 (`FixedN`) aliases
- Conversion, mixed-precision multiplication and division, square root,
  inverse square root, rounding, sine, cosine, and exponential approximation
- Table-driven integer sine (`sin()` returns `FixedN`, exact at +-1, under
  one Q30 LSB error, about 3 ns per call) plus `sinTurns` for angles in turns
- Exact `floor(sqrt(n))` for 32-bit signed integers, with remainder
- `constexpr` support for core fixed-point operations
- CMake targets for subdirectory and installed-package use
- Tested with MSVC, Clang, and GCC; enables AVX2 on x86/x64 when the compiler supports it (skipped on ARM)

Read the focused guides for [fixed-point arithmetic](docs/fixed-point.md),
[fast fixed-point sine](docs/fixed-sine.md),
[integer square root](docs/integer-square-root.md), and
[performance](docs/perf.md).

## Quick example

```cpp
#include "fixedpt/fixed_types.h"
#include "int/isqrt.h"

Fixed16 price{12.5};
Fixed16 quantity{4};
Fixed16 total = price * quantity;

const IntSqrtResult root = integerSqrt(27);
// root.p_ == 5 and root.q_ == 2 because 27 == 5 * 5 + 2.
const IntSqrt64Result root64 = integerSqrt(std::uint64_t{27});
Fixed16 root_fixed = integerSqrt(Fixed16{27}); // same type in and out
```

numlib is available under the [MIT License](LICENSE).

## What you need

- [CMake](https://cmake.org/) 3.20 or newer
- A C++20 compiler (MSVC, Clang, or GCC)
- [Ninja](https://ninja-build.org/) if you use editor IntelliSense (clangd) — see below
- [Cursor](https://cursor.com/) or [VS Code](https://code.visualstudio.com/)
- Recommended extensions (prompted on open; see
  [`.vscode/extensions.json`](.vscode/extensions.json)):
  - **CMake Tools** (`ms-vscode.cmake-tools`)
  - **clangd** (`llvm-vs-code-extensions.vscode-clangd`)

Microsoft’s **C/C++** extension (`ms-vscode.cpptools`) is not required and does
not work in Cursor. Shared [`.vscode/settings.json`](.vscode/settings.json)
disables its IntelliSense so it does not conflict with clangd.

Personal `*.code-workspace` files are gitignored; use the repo folder (and
`.vscode/`) instead.

## Install the toolchain

### Windows

1. Install [Visual Studio](https://visualstudio.microsoft.com/) 2022 or newer (including VS 2026).
2. In the installer, select the **Desktop development with C++** workload (MSVC + CMake).
   That workload also provides Ninja for the **clangd** preset.
3. Open this repo in Cursor/VS Code. CMake Tools will use the Visual Studio kit (no need to put `cl` or `cmake` on your PATH).

### macOS

1. Install the Xcode Command Line Tools:

   ```bash
   xcode-select --install
   ```

2. Install CMake and Ninja if you do not have them (Homebrew):

   ```bash
   brew install cmake ninja
   ```

### Linux

Install CMake, Ninja, and a C++20 compiler with your package manager.

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

1. Open the repo folder in Cursor/VS Code.
2. When prompted, install the recommended extensions (**CMake Tools**, **clangd**).

## Configure and build (editor)

1. Select a CMake kit for your machine (MSVC, Clang, or GCC).
2. Configure with the **default** preset (CMake Tools status bar, or Command Palette → **CMake: Select Configure Preset**).
3. Build (status bar **Build**, or **CMake: Build**).
4. Set the launch/debug target to `test_isqrt`, `test_fixed`, or `perf`, then Run or Debug (F5). Launch configs **Debug test_isqrt**, **Debug test_fixed**, and **Debug perf** are in [`.vscode/launch.json`](.vscode/launch.json).

### Editor IntelliSense (clangd)

Outline, go-to-definition, and diagnostics come from **clangd**. It needs a
[`compile_commands.json`](https://clang.llvm.org/docs/JSONCompilationDatabase.html)
compilation database. The Visual Studio CMake generator does not emit that file;
use the **clangd** Ninja preset once (in addition to your normal `build/` tree):

```bash
cmake --preset clangd
```

That writes `build-clangd/compile_commands.json`. [`.clangd`](.clangd) and
[`.vscode/settings.json`](.vscode/settings.json) already point clangd at that
directory (including `--query-driver` for MSVC on Windows).

After the first configure (or if headers still show “Too many errors emitted”),
run **clangd: Restart language server** from the Command Palette.

You can keep using the Visual Studio generator under `build/` for builds and
debugging; `build-clangd/` is only for the language server.

## Configure and build (CLI)

From the repo root, on any OS:

```bash
# Debug
cmake --preset debug
cmake --build --preset debug

# Release
cmake --preset release
cmake --build --preset release

# Language server database (Ninja; see Editor IntelliSense above)
cmake --preset clangd
```

`default` is an untyped configure preset; prefer `debug` or `release` for
builds. Build presets pass the matching configuration for multi-config
generators (for example Visual Studio).

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

Run `perf` in **Release** for meaningful timings. It uses a small `std::chrono` harness (no Catch2). From the repository root it writes [`docs/perf-results.json`](docs/perf-results.json); regenerate the table with `python scripts/render_perf_md.py` and commit both that JSON and [`docs/perf.md`](docs/perf.md). See [performance](docs/perf.md).

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
