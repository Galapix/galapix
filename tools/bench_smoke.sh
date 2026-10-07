#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Offline smoke for the pixel bench kit:
#   corpus generate (optional) → golden decode → gp-tile → thumtoo-bench --json
#
# Environment:
#   CORPUS_OUT   output/input root (default /tmp/benchtoo-smoke-corpus)
#   CORPUS_GEN   path to gen_synthetic.py (default: sibling ../benchtoo/...)
#   THUMTOO_BUILD_DIR  build dir with tool binaries (default /tmp/thumtoo-build)
#   BENCH_SMOKE_QUICK=1  only photo landscape sample (skip bookpage/comic)
#
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CORPUS_OUT="${CORPUS_OUT:-/tmp/benchtoo-smoke-corpus}"
BUILD_DIR="${THUMTOO_BUILD_DIR:-/tmp/thumtoo-build}"
GEN="${CORPUS_GEN:-}"
if [[ -z "$GEN" && -f "${ROOT}/../benchtoo/generators/gen_synthetic.py" ]]; then
  GEN="${ROOT}/../benchtoo/generators/gen_synthetic.py"
fi

echo "== bench_smoke: corpus =="
if [[ -n "$GEN" && -f "$GEN" ]]; then
  # --no-large keeps smoke fast; classes cover biltoo ebook/comic/album paths
  if [[ "${BENCH_SMOKE_QUICK:-0}" == "1" ]]; then
    python3 "$GEN" --out "$CORPUS_OUT" --no-large --classes photo,landscape
  else
    python3 "$GEN" --out "$CORPUS_OUT" --no-large \
      --classes photo,landscape,bookpage,comic,spread
  fi
  DOC_GEN="$(dirname "$GEN")/gen_documents.py"
  if [[ -f "$DOC_GEN" ]]; then
    echo "== documents (PDF/MD/TXT/CBZ) =="
    python3 "$DOC_GEN" --out "$CORPUS_OUT" --pages 12 --skip-djvu || true
    ls -la "$CORPUS_OUT"/documents 2>/dev/null || true
  fi
  ARCH_GEN="$(dirname "$GEN")/gen_archives.py"
  if [[ -f "$ARCH_GEN" ]]; then
    python3 "$ARCH_GEN" --corpus "$CORPUS_OUT" || true
  fi
else
  echo "CORPUS_GEN not set and sibling benchtoo not found; using CORPUS_OUT=$CORPUS_OUT"
fi

# Prefer representative samples: album photo, portrait book page, comic page
pick_jpeg() {
  local preferred="$1"
  local f
  f=$(ls "$CORPUS_OUT"/synthetic/jpeg/${preferred} 2>/dev/null | head -1 || true)
  if [[ -n "$f" ]]; then
    echo "$f"
    return 0
  fi
  return 1
}

SAMPLES=()
for pref in \
  "photo_1920x1080_q90.jpg" \
  "landscape_1920x1080_q90.jpg" \
  "bookpage_1200x1800_q90.jpg" \
  "comic_1200x1800_q90.jpg" \
  "spread_1920x1080_q90.jpg"
do
  if f=$(pick_jpeg "$pref"); then
    SAMPLES+=("$f")
  fi
done

# Fallbacks if names differ
if [[ ${#SAMPLES[@]} -eq 0 ]]; then
  while IFS= read -r f; do
    SAMPLES+=("$f")
  done < <(ls "$CORPUS_OUT"/synthetic/jpeg/*_q90.jpg 2>/dev/null | head -3 || true)
fi

if [[ ${#SAMPLES[@]} -eq 0 ]]; then
  echo "no JPEGs under $CORPUS_OUT/synthetic/jpeg/" >&2
  exit 1
fi

echo "samples:"
for s in "${SAMPLES[@]}"; do
  echo "  $s ($(wc -c <"$s") bytes)"
done

need() {
  local b="$1"
  if [[ ! -x "$b" ]]; then
    echo "missing executable: $b (build tools first)" >&2
    exit 1
  fi
}

DECODE="${BUILD_DIR}/thumtoo-microbench-decode"
GPTILE="${BUILD_DIR}/thumtoo-gp-tile"
BENCH="${BUILD_DIR}/thumtoo-bench"
command -v thumtoo-microbench-decode >/dev/null 2>&1 && DECODE=$(command -v thumtoo-microbench-decode)
command -v thumtoo-gp-tile >/dev/null 2>&1 && GPTILE=$(command -v thumtoo-gp-tile)
command -v thumtoo-bench >/dev/null 2>&1 && BENCH=$(command -v thumtoo-bench)

need "$DECODE"
need "$GPTILE"

echo "== microbench-decode =="
"$DECODE" --repeat 2 "${SAMPLES[@]}"

echo "== gp-tile (first sample, q=80) =="
"$GPTILE" --repeat 2 --quality 80 "${SAMPLES[0]}"

# Codec comparison needs a lossless source: the PNG twin of the first sample.
PNG_SAMPLE="$CORPUS_OUT/synthetic/png/$(basename "${SAMPLES[0]}" | sed 's/_q[0-9]*\.jpg$/.png/')"
if [[ -f "$PNG_SAMPLE" ]]; then
  echo "== gp-tile --codec all ($(basename "$PNG_SAMPLE")) =="
  "$GPTILE" --repeat 2 --codec all --max-cells 8 "$PNG_SAMPLE" || true
fi

if [[ -x "$BENCH" ]]; then
  echo "== thumtoo-bench --json (first sample) =="
  "$BENCH" --json --ladder 256 --tile-cell \
    --cache /tmp/thumtoo-bench-smoke-cache "${SAMPLES[0]}" | head -c 2500
  echo
else
  echo "skip thumtoo-bench (not built)"
fi

GPARCH="${BUILD_DIR}/thumtoo-gp-archive"
command -v thumtoo-gp-archive >/dev/null 2>&1 && GPARCH=$(command -v thumtoo-gp-archive)
if [[ -x "$GPARCH" ]]; then
  ARCH_SAMPLE=$(ls "$CORPUS_OUT"/archives/*.cbz "$CORPUS_OUT"/documents/sample_book.cbz 2>/dev/null | head -1 || true)
  if [[ -n "$ARCH_SAMPLE" ]]; then
    echo "== gp-archive --backend all =="
    "$GPARCH" --repeat 2 --backend all "$ARCH_SAMPLE" || true
  fi
fi

echo "ok: bench_smoke finished (${#SAMPLES[@]} samples)"
