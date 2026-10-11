#!/usr/bin/env python3
"""Regenerate docs/perf.md from docs/perf-results.json."""

from __future__ import annotations

import json
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
JSON_PATH = REPO_ROOT / "docs" / "perf-results.json"
MD_PATH = REPO_ROOT / "docs" / "perf.md"


def format_ns(value: float) -> str:
    return f"{value:.1f}"


def main() -> int:
    data = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    meta = data["meta"]
    benchmarks = data["benchmarks"]

    lines: list[str] = [
        "# Performance",
        "",
        "Bare-metal timings from the Release `perf` binary. These numbers are",
        "not from GitHub-hosted CI runners. Re-capture on the machine you care",
        "about, then regenerate this page.",
        "",
        "## Machine",
        "",
        f"- **CPU:** {meta.get('cpu', 'unknown')}",
        f"- **OS:** {meta.get('os', 'unknown')}",
        f"- **Captured:** {meta.get('captured_at', 'unknown')}",
        f"- **Samples per batch:** {meta.get('samples', '?')}",
        f"- **Warmup runs:** {meta.get('warmup_runs', '?')}",
        f"- **Timed runs (median):** {meta.get('timed_runs', '?')}",
    ]

    note = meta.get("note")
    if note:
        lines.append(f"- **Note:** {note}")

    lines.extend(
        [
            "",
            "## Results",
            "",
            "| Benchmark | ns/call | ns/batch |",
            "| --- | ---: | ---: |",
        ]
    )

    for row in benchmarks:
        lines.append(
            f"| {row['name']} | {format_ns(row['per_call_ns'])} | "
            f"{format_ns(row['batch_ns'])} |"
        )

    lines.extend(
        [
            "",
            "## Refresh",
            "",
            "From the repository root:",
            "",
            "```text",
            "cmake --preset release",
            "cmake --build --preset release",
            "# Windows (multi-config):",
            "build\\Release\\perf.exe",
            "# macOS / Linux (single-config):",
            "./build/perf",
            "python scripts/render_perf_md.py",
            "```",
            "",
            "Commit both `docs/perf-results.json` and `docs/perf.md`.",
            "",
        ]
    )

    MD_PATH.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"wrote {MD_PATH.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
