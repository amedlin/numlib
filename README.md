# numlib

High-performance numerical algorithms library (C++20).

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
target_link_libraries(your_target PRIVATE numlib)
```

In your sources (C++20):

```cpp
#include "int/isqrt.h"
#include "fixedpt/fixed_types.h"
```

## Add implementation files

1. Add the `.cpp` under the right topic directory (for example `int/`).
2. List it in `target_sources` for the `numlib` target in `CMakeLists.txt`.

Headers hold the public API and anything needed for `constexpr`. Non-trivial runtime code goes in `.cpp` files. See [FORMAT.md](FORMAT.md) for naming and layout rules.
