#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare current golden-tool JSON against a baseline snapshot.

Timing keys (name ends with _ms or is encode_ms/decode_ms) use --tolerance-pct
(default 25); only slowdowns count. Byte-like keys use --bytes-tolerance-pct
(default 5) in both directions.

List elements are matched by identity, not position: an object carrying
identity fields (variant/codec, backend, quality, name, file, archive) is keyed
as e.g. ``codecs[codec=jpeg].rows[quality=80].encode_ms`` or, in a batch
document, ``results[archive=/data/a.cbz].variants[backend=unarr].metrics...``. Inserting a row or a
codec therefore does not shift every later key. Elements without identity
fields, or with duplicate identities, fall back to their index.

For comparison documents ("kind": "compare") a change of verdict winner is
reported; with --fail-on-winner-change a change of the *overall* (or aggregate)
winner is an error. Per-metric winners are only listed: on near-tie metrics they
flip between identical runs, so gating on them would be flaky. Batch documents
("kind": "batch") report each result's winners under its identity, plus the
aggregate winner.

Exit: 0 within tolerance, 1 regression (or winner change when asked),
2 usage / unreadable input.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

# Identity fields, in key order. "variant" (e.g. "jxl@e1") supersedes "codec".
IDENTITY_FIELDS = ("variant", "codec", "backend", "quality", "name", "file", "archive")


def identity(obj: Any) -> str | None:
    if not isinstance(obj, dict):
        return None
    parts = []
    for field in IDENTITY_FIELDS:
        if field == "codec" and "variant" in obj:
            continue
        value = obj.get(field)
        if isinstance(value, (str, int)) and not isinstance(value, bool):
            parts.append(f"{field}={value}")
    return ",".join(parts) or None


def walk(obj: Any, prefix: str = "") -> dict[str, float]:
    out: dict[str, float] = {}
    if isinstance(obj, dict):
        for k, v in obj.items():
            p = f"{prefix}.{k}" if prefix else k
            if isinstance(v, (int, float)) and not isinstance(v, bool):
                out[p] = float(v)
            else:
                out.update(walk(v, p))
    elif isinstance(obj, list):
        ids = [identity(v) for v in obj]
        for i, (v, ident) in enumerate(zip(obj, ids)):
            unique = ident is not None and ids.count(ident) == 1
            out.update(walk(v, f"{prefix}[{ident if unique else i}]"))
    return out


def is_timing(key: str) -> bool:
    k = key.rsplit(".", 1)[-1]
    return k.endswith("_ms") or k in ("encode_ms", "decode_ms", "median_ms")


def is_bytes(key: str) -> bool:
    k = key.rsplit(".", 1)[-1]
    return "byte" in k.lower() or k in ("bytes_total", "jpeg_bytes", "members")


def winners(doc: Any, prefix: str = "") -> dict[str, str]:
    """Verdict winners of a comparison or batch document.

    {"overall": name, metric: name} for a comparison; for a batch the same per
    result, prefixed with the result's identity, plus "aggregate".
    """
    if not isinstance(doc, dict):
        return {}
    out: dict[str, str] = {}
    verdict = doc.get("verdict")
    if isinstance(verdict, dict):
        overall = verdict.get("overall", {})
        if isinstance(overall, dict):
            out[prefix + "overall"] = overall.get("winner", "")
        metrics = verdict.get("metrics", {})
        if isinstance(metrics, dict):
            for name, m in metrics.items():
                if isinstance(m, dict):
                    out[prefix + name] = m.get("winner", "")
    results = doc.get("results")
    if isinstance(results, list):
        ids = [identity(r) for r in results]
        for i, (r, ident) in enumerate(zip(results, ids)):
            unique = ident is not None and ids.count(ident) == 1
            out.update(winners(r, f"{prefix}[{ident if unique else i}] "))
    aggregate = doc.get("aggregate")
    if isinstance(aggregate, dict) and "winner" in aggregate:
        out[prefix + "aggregate"] = aggregate["winner"]
    return out


def load(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as e:
        print(f"cannot read {path}: {e}", file=sys.stderr)
        raise SystemExit(2)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--baseline", type=Path, required=True)
    ap.add_argument("--current", type=Path, required=True)
    ap.add_argument("--tolerance-pct", type=float, default=25.0)
    ap.add_argument("--bytes-tolerance-pct", type=float, default=5.0)
    ap.add_argument("--fail-on-winner-change", action="store_true",
                    help="treat a changed overall (or aggregate) winner as a "
                         "regression; per-metric changes are only reported")
    args = ap.parse_args()

    base = load(args.baseline)
    cur = load(args.current)
    bf = walk(base)
    cf = walk(cur)

    regressions = []
    missing = []
    for key, bv in bf.items():
        if key not in cf:
            if is_timing(key) or is_bytes(key):
                missing.append(key)
            continue
        cv = cf[key]
        if bv == 0:
            if cv != 0 and (is_timing(key) or is_bytes(key)):
                regressions.append((key, bv, cv, float("inf")))
            continue
        pct = abs(cv - bv) / abs(bv) * 100.0
        lim = args.tolerance_pct if is_timing(key) else (
            args.bytes_tolerance_pct if is_bytes(key) else None
        )
        if lim is None:
            continue
        # Only flag slowdowns for timings (current much larger)
        if is_timing(key) and cv > bv and pct > lim:
            regressions.append((key, bv, cv, pct))
        elif is_bytes(key) and pct > lim:
            regressions.append((key, bv, cv, pct))

    changed = []
    bw, cw = winners(base), winners(cur)
    for metric, before in bw.items():
        after = cw.get(metric)
        if after is not None and after != before:
            changed.append((metric, before or "tie", after or "tie"))

    if missing:
        print("missing keys in current:", ", ".join(missing), file=sys.stderr)
    if changed:
        print("verdict winner changed:", file=sys.stderr)
        for metric, before, after in changed:
            print(f"  {metric}: {before} -> {after}", file=sys.stderr)
    if regressions:
        print("regressions:", file=sys.stderr)
        for key, bv, cv, pct in regressions:
            print(f"  {key}: baseline={bv:.4g} current={cv:.4g} delta={pct:.1f}%",
                  file=sys.stderr)
        return 1
    fatal = [c for c in changed if c[0].endswith(("overall", "aggregate"))]
    if fatal and args.fail_on_winner_change:
        return 1
    print("ok: within tolerance")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
