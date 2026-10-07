#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Capture golden-tool JSON baselines for a machine class.
#
# Usage:
#   MACHINE=nixx86-zen3 CORPUS=/path/to/benchtoo-out \
#     ./tools/capture_baselines.sh
#
# Writes docs/bench/baselines/$MACHINE/*.json
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MACHINE="${MACHINE:-$(uname -s)-$(uname -m)}"
OUT="${BASELINE_OUT:-$ROOT/docs/bench/baselines/$MACHINE}"
CORPUS="${CORPUS:-${CORPUS_OUT:-/tmp/benchtoo-smoke-corpus}}"
BUILD_DIR="${THUMTOO_BUILD_DIR:-/tmp/thumtoo-build}"
REPEAT="${REPEAT:-5}"

mkdir -p "$OUT"

find_bin() {
  local name="$1"
  if [[ -x "$BUILD_DIR/$name" ]]; then echo "$BUILD_DIR/$name"; return; fi
  if command -v "$name" >/dev/null 2>&1; then command -v "$name"; return; fi
  echo "missing $name" >&2
  exit 1
}

DECODE=$(find_bin thumtoo-microbench-decode)
TILE=$(find_bin thumtoo-gp-tile)
ARCH=$(find_bin thumtoo-gp-archive)

JPEG=$(ls "$CORPUS"/synthetic/jpeg/photo_*_q90.jpg 2>/dev/null | head -1 || true)
if [[ -z "$JPEG" ]]; then
  JPEG=$(ls "$CORPUS"/synthetic/jpeg/*_q90.jpg 2>/dev/null | head -1 || true)
fi
CBZ=$(ls "$CORPUS"/archives/*.cbz 2>/dev/null | head -1 || true)
if [[ -z "$CBZ" ]]; then
  CBZ=$(ls "$CORPUS"/documents/sample_book.cbz 2>/dev/null | head -1 || true)
fi

# Lossless source for the codec comparison (a JPEG source favors JPEG).
PNG=$(ls "$CORPUS"/synthetic/png/photo_*.png 2>/dev/null | head -1 || true)

if [[ -z "$JPEG" ]]; then
  echo "no JPEG under $CORPUS — generate corpus first" >&2
  exit 1
fi

echo "machine=$MACHINE out=$OUT"
echo "jpeg=$JPEG"
[[ -n "$CBZ" ]] && echo "cbz=$CBZ"

"$DECODE" --json --repeat "$REPEAT" "$JPEG" >"$OUT/microbench-decode.json"
"$TILE" --json --codec jpeg --quality 80 --repeat "$REPEAT" --max-cells 8 "$JPEG" \
  >"$OUT/gp-tile-jpeg.json"
if [[ -n "$CBZ" ]]; then
  "$ARCH" --json --backend libarchive --repeat "$REPEAT" "$CBZ" \
    >"$OUT/gp-archive-libarchive.json"
fi

# Verdict snapshots (who wins on this machine). compare_bench_json matches
# rows by identity and reports winner changes (--fail-on-winner-change).
if [[ -n "$PNG" ]]; then
  # Default efforts plus the fast ones an interactive tile encoder would use.
  "$TILE" --json --codec all,webp@e0,jxl@e1,jxl@e3 --repeat "$REPEAT" --max-cells 8 "$PNG" \
    >"$OUT/gp-tile-compare.json" ||
    echo "warning: gp-tile --codec all reached no verdict (see gp-tile-compare.json)" >&2
fi
if [[ -n "$CBZ" ]]; then
  "$ARCH" --json --backend all --repeat "$REPEAT" "$CBZ" \
    >"$OUT/gp-archive-compare.json" ||
    echo "warning: gp-archive --backend all reached no verdict (see gp-archive-compare.json)" >&2
fi

# meta
cat >"$OUT/README.md" <<META
<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Baseline: $MACHINE

Captured $(date -u +%Y-%m-%dT%H:%MZ) via tools/capture_baselines.sh

- JPEG: $(basename "$JPEG")
- PNG (codec comparison): $(basename "${PNG:-none}")
- CBZ: $(basename "${CBZ:-none}")
- repeats: $REPEAT

Compare:

\`\`\`bash
python3 tools/compare_bench_json.py \\
  --baseline docs/bench/baselines/$MACHINE/gp-tile-jpeg.json \\
  --current /tmp/gp-tile.json
\`\`\`

\`gp-tile-compare.json\` / \`gp-archive-compare.json\` are verdict snapshots
(which codec / backend wins here). compare_bench_json reports winner changes;
add --fail-on-winner-change to gate on them.
META

echo "wrote $OUT"
ls -la "$OUT"
