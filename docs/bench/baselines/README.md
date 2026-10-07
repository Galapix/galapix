<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Benchmark baselines

Store golden-tool JSON snapshots per machine class for regression detection.

## Layout

```
docs/bench/baselines/<machine-class>/
  microbench-decode.json
  gp-tile-jpeg.json
  gp-archive-libarchive.json
  gp-tile-compare.json        # verdict snapshot: which codec wins (PNG source)
  gp-archive-compare.json     # verdict snapshot: which backend wins
```

The `*-compare.json` files record which variant won on that machine.
`compare_bench_json` matches list elements by identity (codec/variant,
backend, quality, …), so they compare like any other file; it also lists
verdict winner changes. `--fail-on-winner-change` fails the run only when the
**overall** (or, for batch documents, aggregate) winner changes: per-metric
winners flip between identical runs on near-tie metrics, so they are listed
but never fatal. Batch documents (several inputs per run) are keyed by
`archive` / `file`.
Refined codec match rows can differ in quality between runs; those show up
as missing keys (reported, not fatal).

## Capture

Preferred:

```bash
# after building tools and generating corpus (or nix build github:Grumbel/benchtoo#corpus-smoke)
CORPUS=$(nix build --print-out-paths github:Grumbel/benchtoo#corpus-smoke)
MACHINE=nixx86-ref CORPUS="$CORPUS" ./tools/capture_baselines.sh
```

Manual:

```bash
thumtoo-microbench-decode --json --repeat 5 FILE > microbench-decode.json
thumtoo-gp-tile --json --codec jpeg --quality 80 FILE > gp-tile-jpeg.json
thumtoo-gp-archive --json --backend libarchive ARCHIVE > gp-archive-libarchive.json
```

## Compare

```bash
python3 tools/compare_bench_json.py \
  --baseline docs/bench/baselines/nixx86-ref/gp-tile-jpeg.json \
  --current /tmp/gp-tile.json \
  --tolerance-pct 25
```

Exit code 0 = within tolerance; 1 = regression (or overall winner change with
`--fail-on-winner-change`); 2 = usage / unreadable input.

Keys look like `codecs[codec=jpeg].rows[codec=jpeg,quality=80].encode_ms`;
elements without identity fields (or with duplicates) fall back to `[index]`.
Self-test: `python3 tools/test_compare_bench_json.py`.

Tolerances are relative on timing fields (`*_ms`, `encode_ms`, `decode_ms`).
Byte counts are compared with a smaller default tolerance (5%).

See `example/` for schema-only placeholders (not for CI gates).
