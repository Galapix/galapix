#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Self-test for compare_bench_json.py (run by checks.baseline-compare-tool)."""

from __future__ import annotations

import copy
import json
import subprocess
import sys
import tempfile
from pathlib import Path

TOOL = Path(__file__).with_name("compare_bench_json.py")


def run(base: object, cur: object, *extra: str) -> tuple[int, str]:
    with tempfile.TemporaryDirectory() as d:
        b = Path(d, "base.json")
        c = Path(d, "cur.json")
        b.write_text(json.dumps(base))
        c.write_text(json.dumps(cur))
        p = subprocess.run([sys.executable, str(TOOL), "--baseline", str(b),
                            "--current", str(c), *extra],
                           capture_output=True, text=True)
        return p.returncode, p.stdout + p.stderr


def row(codec: str, q: int, enc: float, nbytes: int, **extra: object) -> dict:
    return {"codec": codec, "quality": q, "encode_ms": enc, "bytes_total": nbytes, **extra}


FAILS = 0


def expect(cond: bool, msg: str, detail: str = "") -> None:
    global FAILS
    if not cond:
        FAILS += 1
        print(f"FAIL: {msg}\n{detail}", file=sys.stderr)


def main() -> int:
    base = {"rows": [row("jpeg", 60, 1.0, 100), row("jpeg", 80, 2.0, 200)]}

    rc, out = run(base, base)
    expect(rc == 0, "identity passes", out)

    # Inserting a row before the others must not shift keys.
    shifted = {"rows": [row("jpeg", 40, 9.0, 50)] + base["rows"]}
    rc, out = run(base, shifted)
    expect(rc == 0, "inserted row matched by identity", out)

    # A real slowdown on the q80 row is still caught, under its identity key.
    slow = copy.deepcopy(base)
    slow["rows"][1]["encode_ms"] = 4.0
    rc, out = run(base, slow)
    expect(rc == 1 and "rows[codec=jpeg,quality=80].encode_ms" in out,
           "slowdown flagged by identity key", out)

    # Speedups are not regressions; byte changes are, in both directions.
    fast = copy.deepcopy(base)
    fast["rows"][1]["encode_ms"] = 0.5
    expect(run(base, fast)[0] == 0, "speedup ok")
    smaller = copy.deepcopy(base)
    smaller["rows"][1]["bytes_total"] = 150
    expect(run(base, smaller)[0] == 1, "byte change flagged")

    # Effort variants key by "variant", distinct from the default codec.
    var = {"rows": [row("jxl", 80, 1.0, 100), row("jxl", 80, 9.0, 100,
                                                     variant="jxl@e1", effort=1)]}
    var_swapped = {"rows": list(reversed(var["rows"]))}
    expect(run(var, var_swapped)[0] == 0, "variant rows matched by variant")

    # Duplicate identities fall back to positional keys.
    dup = {"rows": [row("jpeg", 80, 1.0, 100), row("jpeg", 80, 1.0, 100)]}
    expect(run(dup, dup)[0] == 0, "duplicate identities fall back to index")

    # Verdict winner changes are reported; fatal only on request.
    v1 = {"kind": "compare", "verdict": {"metrics": {"bytes_total": {"winner": "jxl"}},
                                         "overall": {"winner": "jpeg"}}}
    v2 = copy.deepcopy(v1)
    v2["verdict"]["metrics"]["bytes_total"]["winner"] = "webp"
    rc, out = run(v1, v2)
    expect(rc == 0 and "bytes_total: jxl -> webp" in out, "metric winner change reported", out)
    rc, out = run(v1, v2, "--fail-on-winner-change")
    expect(rc == 0, "per-metric flips are informational even with the flag", out)
    v3 = copy.deepcopy(v1)
    v3["verdict"]["overall"]["winner"] = "webp"
    rc, out = run(v1, v3)
    expect(rc == 0 and "overall: jpeg -> webp" in out, "overall change reported", out)
    rc, out = run(v1, v3, "--fail-on-winner-change")
    expect(rc == 1, "overall winner change fatal when requested", out)

    # Batch documents: results keyed by archive, winners per archive + aggregate.
    def result(archive: str, winner: str, ms: float) -> dict:
        return {"kind": "compare", "archive": archive,
                "variants": [{"backend": "unarr", "metrics": {"toc_ms": ms}}],
                "verdict": {"metrics": {"toc_ms": {"winner": winner}},
                            "overall": {"winner": winner}}}

    b1 = {"kind": "batch", "results": [result("/d/a.cbz", "unarr", 1.0),
                                       result("/d/b.cbz", "unarr", 2.0)],
          "aggregate": {"winner": "unarr"}}
    # Same data, results listed in the other order: still identical.
    b_swapped = copy.deepcopy(b1)
    b_swapped["results"].reverse()
    rc, out = run(b1, b_swapped, "--fail-on-winner-change")
    expect(rc == 0, "batch results matched by archive", out)
    # b.cbz got slower: flagged under its archive identity.
    b_slow = copy.deepcopy(b1)
    b_slow["results"][1]["variants"][0]["metrics"]["toc_ms"] = 9.0
    rc, out = run(b1, b_slow)
    expect(rc == 1 and "results[archive=/d/b.cbz].variants[backend=unarr].metrics.toc_ms" in out,
           "batch regression keyed by archive", out)
    # Winner flips for one archive and for the aggregate.
    b_flip = copy.deepcopy(b1)
    b_flip["results"][0]["verdict"]["overall"]["winner"] = "libarchive"
    b_flip["aggregate"]["winner"] = ""
    rc, out = run(b1, b_flip, "--fail-on-winner-change")
    expect(rc == 1 and "[archive=/d/a.cbz] overall: unarr -> libarchive" in out
           and "aggregate: unarr -> tie" in out, "batch winner changes reported", out)

    # Unreadable input is a usage error.
    with tempfile.TemporaryDirectory() as d:
        bad = Path(d, "bad.json")
        bad.write_text("{not json")
        p = subprocess.run([sys.executable, str(TOOL), "--baseline", str(bad),
                            "--current", str(bad)], capture_output=True, text=True)
        expect(p.returncode == 2, "bad json -> exit 2", p.stderr)

    if FAILS:
        print(f"{FAILS} failure(s)", file=sys.stderr)
        return 1
    print("ok: compare_bench_json self-test")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
