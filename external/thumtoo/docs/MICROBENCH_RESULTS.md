# Microbench results (Pillow / system unzip)

Environment: PIL decode uses libjpeg DCT scaling via `Image.draft`.
Not identical to `vips_jpegload(shrink=N)` but same 1/2/4/8 factors.


## Verified under libvips (2026-10-01)

Environment: Ubuntu system libvips 8.15.1, greyscale synthetic corpus from
**benchtoo** (https://github.com/Grumbel/benchtoo) (`vips grey` + `jpegsave Q=90`). Tool:
`thumtoo-microbench-decode` (no Client). Medians, 5 repeats + 1 warmup.

| file | MP | size_ms | full_ms | shrink2 | shrink4 | shrink8 | thumb32 | thumb256 |
|------|----|---------|---------|---------|---------|---------|---------|----------|
| synth_800x600_q90.jpg | 0.48 | 0.18 | 0.58 | 0.24 | 0.18 | 0.15 | 2.56 | 5.29 |
| synth_1920x1080_q90.jpg | 2.07 | 0.21 | 1.68 | 0.44 | 0.27 | 0.21 | 2.70 | 5.07 |
| synth_3840x2160_q90.jpg | 8.29 | 0.31 | 4.02 | 1.03 | 0.30 | 0.22 | 5.95 | 8.01 |
| synth_7680x4320_q90.jpg | 33.18 | 0.18 | 10.88 | 1.01 | 0.44 | 0.22 | 15.83 | 16.70 |

### Interpretation (vips, this run)

- **Size-only** remains ~0.2 ms.
- **Full decode** scales roughly with pixels (0.6 → 11 ms on this greyscale corpus).
- **`vips_jpegload(shrink=N)`** is much stronger than the earlier Pillow `draft`
  numbers: at 33 MP, shrink8 ≈ **49×** faster than full (0.22 vs 10.9 ms).
  Shrink helps more on large files; entropy + IDCT both shrink.
- **`vips_thumbnail`** is *not* pure shrink-on-load cost here (2–17 ms) — still
  useful for LQIP edges but slower than a direct shrink=8 load for overview.

### `thumtoo-bench` smoke (same 2.1 MP JPEG)

Cold (`--json --ladder 256 --tile-cell`): probe ~10 ms, preview/JXL ~67 ms,
tile-cell ~11 ms, total ~107 ms (2 workers, Release).

Warm (`--keep-cache --ladder 0 --tile-cell`): probe ~0.2 ms, tile-cell ~7 ms.

See [BENCHMARK_KIT.md](BENCHMARK_KIT.md) for the full kit plan.

---

## JPEG decode

| file | MP | size_only ms | full ms | draft/2 ms | × | draft/4 | × | draft/8 | × |
|------|----|--------------|---------|------------|---|--------|---|---------|---|
| synth_1920x1080.jpg | 2.1 | 0.03 | 22.4 | 17.1 | 1.3 | 16.2 | 1.4 | 13.4 | 1.7 |
| synth_3840x2160.jpg | 8.3 | 0.03 | 85.9 | 65.6 | 1.3 | 60.6 | 1.4 | 53.5 | 1.6 |
| synth_7680x4320.jpg | 33.2 | 0.03 | 359.5 | 275.8 | 1.3 | 248.4 | 1.4 | 210.8 | 1.7 |
| synth_800x600.jpg | 0.5 | 0.03 | 2.3 | 1.6 | 1.4 | 1.1 | 2.0 | 1.0 | 2.3 |

## Interpretation (2026-09-09, this environment)

- **Size-only is essentially free** (~0.03 ms) vs full decode (2–360 ms on this corpus).
- Pillow `Image.draft` (libjpeg DCT scale) only ~**1.3–2.3×** faster than full load
  on these files — Huffman/entropy decode still dominates; pixel IDCT shrink helps
  less than a naive pixel-count argument suggests. Expect **Vips `jpegload(shrink=N)`**
  to be in a similar ballpark (measure in-tree next).
- PSNR of draft vs high-quality downsample is modest (22–32 dB) — fine for
  coarse tiles / overview, not for scale-0 HQ.
- PIL `thumbnail(32)` on 8K is **~207 ms** (full decode then resize). If LQIP used
  this pattern it would be slow; **Vips `vips_thumbnail` uses shrink-on-load for JPEG**
  and should be much faster — must confirm with `thumtoo`/vips microbench under nix.
- ZIP (stored): random member via Python zipfile is cheap; `unzip -p` process
  spawn is slower than in-process (~2.6 ms vs 0.2 ms). libarchive sequential walk
  cost will dominate on large RAR/solid archives (not in this corpus yet).

## Gaps still to measure under full thumtoo/vips build

1. `vips_jpegload` shrink=1/2/4/8 wall + RSS
2. `vips_thumbnail` edge 32/256 vs draft
3. EXIF embedded thumb extract + decode
4. `build_tile_cell` file path vs buffer with/without decode_cache_key
5. libarchive vs unzip vs unrar on large RAR
6. Warm `get_tile` latency (SQLite + JPEG decode of 256² only)


## Archive extract (2026-09-09, Python zipfile / unzip)

Corpus under `/tmp/bench_corpus` (regenerable).

### ZIP stored, 200× 640×480 JPEG (~37 MB archive)

| Op | median ms |
|----|-----------|
| TOC `namelist` | 0.50 |
| extract all sequential | 16.8 |
| extract first member | 0.58 |
| extract last member | 0.58 |
| ~10 scattered members | 1.24 |
| `unzip -l` | 2.73 |
| `unzip -p` last | 2.77 |

Stored ZIP: random member ≈ sequential first member — central directory allows
seek. Process spawn (`unzip -p`) costs ~2 ms extra vs in-process.

### ZIP deflate, 50× 1280×720 JPEG

| Op | median ms |
|----|-----------|
| TOC | 0.14 |
| extract all sequential | 145 |
| extract first | 3.0 |
| extract last | 3.2 |
| ~10 scattered | 29.5 |
| `unzip -p` last | 7.6 |

Deflate: still near-random access per member for non-solid ZIP; cost is
inflate, not full-archive scan. **Solid RAR** is different (must decode prior
members) — not measured here (no `unrar` in sandbox).

### Implications for thumtoo

- Warm path that skips extract when `get_tile` hits (client coalesce) is
  essential for large galleries — already implemented.
- Cold open of deflate ZIP is dominated by inflate×members + image probe/LQIP,
  not TOC.
- libarchive sequential multi-member extract in one pass matches “extract all”
  class of cost; better than N process spawns.
