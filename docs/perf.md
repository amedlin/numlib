# Performance

Bare-metal timings from the Release `perf` binary. These numbers are
not from GitHub-hosted CI runners. Re-capture on the machine you care
about, then regenerate this page.

## Machine

- **CPU:** AMD Ryzen 7 3700X 8-Core Processor
- **OS:** Windows
- **Captured:** 2026-10-14T11:34:00Z
- **Samples per batch:** 1000
- **Warmup runs:** 20
- **Timed runs (median):** 200
- **Note:** Bare-metal baseline; Release perf writes this file

## Results

| Benchmark | ns/call | ns/batch |
| --- | ---: | ---: |
| int-to-float + std::sqrt | 1.3 | 1300.0 |
| integerSqrt | 2.5 | 2500.0 |
| detail::constexprSqrt | 52.0 | 52000.0 |
| integerSqrt uint64 | 19.1 | 19100.0 |
| float add | 0.7 | 700.0 |
| Fixed16 add | 0.4 | 400.0 |
| float mul | 0.7 | 700.0 |
| Fixed16 mul | 0.6 | 600.0 |
| float div | 0.8 | 800.0 |
| Fixed16 div | 5.2 | 5200.0 |
| float mul-add | 0.7 | 700.0 |
| Fixed16 mulAdd | 0.7 | 700.0 |
| float sqrt | 1.3 | 1300.0 |
| Fixed16 sqrt | 3.6 | 3600.0 |
| float sin | 5.3 | 5300.0 |
| Fixed16 sin (CORDIC) | 87.7 | 87700.0 |
| Fixed16 sin (LUT, FixedN result) | 3.0 | 3000.0 |
| sinTurns (LUT, FixedN result) | 2.7 | 2700.0 |
| float cos | 5.2 | 5200.0 |
| Fixed16 cos (LUT, FixedN result) | 3.1 | 3100.0 |
| Fixed16 sinCos (LUT, FixedN result) | 4.8 | 4800.0 |
| cosTurns (LUT, FixedN result) | 2.8 | 2800.0 |
| Fixed16 sinViaFloat | 6.5 | 6500.0 |

## Refresh

From the repository root:

```text
cmake --preset release
cmake --build --preset release
# Windows (multi-config):
build\Release\perf.exe
# macOS / Linux (single-config):
./build/perf
python scripts/render_perf_md.py
```

Commit both `docs/perf-results.json` and `docs/perf.md`.
