# numlib format specification

## Naming

| Kind | Style | Examples |
|------|--------|----------|
| Namespaces | lowerCamelCase | `detail` |
| Functions | lowerCamelCase | `integerSqrt`, `makeSqrtTable` |
| Types (structs, classes, enums, aliases) | PascalCase | `IntSqrtResult` |
| Class / struct data members | snake_case with trailing `_` | `p_`, `q_` |
| Compile-time constants (`constexpr`) | SCREAMING_SNAKE_CASE | `SQRT_TABLE`, `NUMERATOR` |
| Local variables and function arguments | snake_case | `input`, `x_q30`, `denominator` |

Do not use `snake_case` or `SCREAMING_SNAKE_CASE` for namespaces or functions.

## Layout

- Indent with 4 spaces; do not use tabs.
- Use Allman braces: opening `{` on its own line for functions, types, namespaces, and control statements.
- Prefer `const` and `constexpr` where values do not change.
- Prefer `[[nodiscard]]` on functions whose return value must not be ignored.
- Prefer `noexcept` on non-throwing functions.
- Prefer fixed-width integer types from `<cstdint>` (`std::uint32_t`, `std::uint64_t`, …).

## Headers and sources

- Put the public API and anything required for `constexpr` in headers under topic directories (e.g. `int/isqrt.h`).
- Put non-trivial runtime implementation in `.cpp` files and list them in `target_sources` for the `numlib` static library in `CMakeLists.txt`.
- Use `#pragma once` in public headers.
- Include only what the translation unit needs; order: corresponding header (if any), then standard library, then project headers.
