<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Benchmark kit — plan

**Status:** usable kit (2026-10-01). Golden tools, bench_smoke, checks.bench-smoke-lite (corpus-smoke), capture_baselines.sh, compare_bench_json. Automatic comparisons: `gp-archive --backend all`, `gp-tile --codec all` (§6.5). Capture real machine baselines next; example/ is schema only.
**Audience:** agents and humans choosing pixel / archive / codec routes for
thumtoo + biltoo, and detecting regressions against those choices.

Related live docs:

- [PERFORMANCE.md](https://github.com/Grumbel/biltoo/blob/master/docs/PERFORMANCE.md) (order-of-magnitude model)
- [TTFP.md](https://github.com/Grumbel/biltoo/blob/master/docs/TTFP.md) (host first-pixel stages)
- [PIXEL_AND_ARCHIVE_POLICY.md](PIXEL_AND_ARCHIVE_POLICY.md) (tiles durable; soft ephemeral; Random vs Sequential)
- [TILES.md](../TILES.md) (256² grid, default JPEG q=80)
- [MICROBENCH_RESULTS.md](MICROBENCH_RESULTS.md) (early Pillow / zipfile numbers; gaps listed)
- Existing tools: `thumtoo-bench`, `tools/microbench_decode.cpp`,
  `tools/microbench_archive.py`, biltoo `BILTOO_TTFP=1` / PerformancePanel

---

## 1. Goals

1. **Judgement, not vanity numbers.** For each decision we already made or
   still face (tile codec, archive backend, shrink-on-load vs full decode,
   RGB vs RGBA, Store round-trips vs pure decode), produce comparable numbers
   that answer *when route A beats route B* and *by how much*.
2. **Regression target.** A fixed corpus + fixed harness so that a future
   change that makes warm `get_tile` 3× slower is obvious in CI or a local
   `nix run`.
3. **Golden paths outside the library.** Minimal reference programs that do
   “the obvious correct thing” with vips / libjpeg / libjxl / libarchive /
   libunarr **without** going through `thumtoo::Client` or SQLite. If the
   library is slower than the golden path by more than a documented budget,
   that is overhead we must justify or remove.
4. **Manual inspection + automatic flake output.** Developers can drill into
   one dimension (`codec matrix`, `one archive member random vs sequential`);
   CI / `nix flake check` can run a short subset that fails on gross regression.
5. **Separate corpus flake.** Test data must not live in the thumtoo or
   biltoo source trees. Generate synthetic assets where possible; pull
   public-domain samples (archive.org / Wikimedia) as a pinned flake input.

Non-goals (for v1):

- Full GUI frame timing / vsync (biltoo PerformancePanel already samples some
  of this; keep host-level TTFP there).
- Network / HTTP content-id resolve latency.
- OCR / text pipeline timing (separate suite later).
- Claiming absolute milliseconds across machines — always report relative
  ratios and the machine fingerprint.

---

## 2. Inventory of what already exists

| Asset | What it measures | Gaps |
|-------|------------------|------|
| `thumtoo-bench` | Cold-cache phases via full Client: size probe → ladder/JXL preview → optional single tile / pyramid | Tied to library; no codec matrix; no golden baseline; cache wipe only |
| `microbench_decode.cpp` | Standalone decode timing (Pillow-era path; needs vips alignment) | Not built as a first-class flake app; incomplete vs libjpeg shrink |
| `microbench_archive.py` | ZIP TOC / sequential / random / `unzip` CLI | No libarchive C++ path, no unarr, no solid RAR, no 7z |
| `MICROBENCH_RESULTS.md` | Snapshot of early numbers + explicit gap list | Stale relative to Store-only / tile-first policy |
| biltoo `BILTOO_TTFP` | Host stage deltas to first `installDisplayPixels` | Not a library microbench; good for end-to-end golden *product* path |
| biltoo PerformancePanel / `perfstats.h` | Runtime HUD counters | Not batch / offline |
| `BuildStats` in thumtoo | CPU split jpeg/jxl/… during worker work | Good for post-run attribution; not a controlled matrix |

**Conclusion:** we have useful *smoke* tools and one host TTFP path. We lack:

- a codec matrix harness,
- archive backend A/B (libarchive vs libunarr, random vs sequential),
- pure golden-path binaries independent of Client/Store,
- a versioned corpus flake,
- machine-readable result format + baseline comparison,
- flake `apps` / `checks` that run a stable subset.

---

## 3. Decision questions the kit must answer

### 3.1 Tile storage codec

Default today: **JPEG q=80** for 256² tiles (`kDefaultTileCodec`). Alternatives
to quantify:

| Codec | Why it matters |
|-------|----------------|
| JPEG (q=60..95) | Baseline; universal decode |
| WebP (lossy) | Size vs decode CPU on small tiles |
| AVIF | Size; encode often too slow for interactive; still measure |
| JPEG XL (lossy effort 1..3; lossless JPEG recompress) | Soft path already uses JXL; tiles may or may not win |
| RGB888 / raw | Upper bound on decode speed; disk cost |

Metrics per cell and per full scale-0 grid (e.g. 8K image → many tiles):

- encode wall ms, decode wall ms (cold and warm file/OS cache)
- compressed bytes
- optional quality proxy (PSNR / SSIM vs source tile) — secondary

**Judgement target:** pick default tile codec + quality for interactive zoom
(encode on first visit must not dominate; decode of one 256² cell must stay
sub-millisecond class on warm cache).

### 3.2 Sequential vs random archive access

Policy already distinguishes `ArchiveAccess::Random` vs `Sequential`
([PIXEL_AND_ARCHIVE_POLICY.md](PIXEL_AND_ARCHIVE_POLICY.md) §2). Measure:

- ZIP stored, ZIP deflate, non-solid 7z, solid 7z, RAR4 solid (unarr), RAR5
  (libarchive fallback), tar/tar.gz
- TOC time, extract-first, extract-last, extract-scattered-N, extract-all
- Same workload through **libarchive only**, **libunarr only** (where
  applicable), and the **thumtoo dispatcher** (prefers unarr for RAR4)

**Judgement target:** confirm Random → on-demand + LRU is correct for CBZ;
Sequential → cursor/window is mandatory for solid RAR; quantify unarr win on
RAR4 solid vs libarchive.

### 3.3 JPEG scale (shrink-on-load) vs full decode

- `vips_jpegload` shrink 1/2/4/8 (and thumbnail edge 32/256/512/1024)
- libjpeg-turbo DCT scale if exposed
- Full decode + resize (anti-pattern to measure as upper bound)
- EXIF/embedded thumbnail extract + decode (when present)

**Judgement target:** when is shrink-on-load “enough” for overview / PreferCache
plateau vs when must we pay full decode (FocusFull / scale-0 tiles).

### 3.4 Full-frame encode/decode vs tile chunking

- Full JPEG (or JXL) encode/decode of W×H vs building/decoding only the
  visible 256² cells at the needed scale
- Assemble TileSynth (N cell decodes + blit) vs one full-frame decode

**Judgement target:** zoom/pan should stay on tiles; document the break-even
where TileSynth of a coarse complete scale beats re-decoding a soft frame.

### 3.5 Total time-to-pixels (stage breakdown)

Stages to instrument in both golden path and thumtoo Client path:

1. Path → size (header / light probe)
2. Embedded thumbnail (if any)
3. Archive TOC + member extract (if archive)
4. JPEG shrink / overview
5. Full decode
6. Tile cell build (decode source region → encode tile blob)
7. Store / SQLite round-trip (put + get blob)
8. Host: blob → QImage / RGB buffer → screen (see §3.6)

Cold vs hot cache for each stage. Report median + p95 over fixed repeats.

### 3.6 RGB24 vs RGBA32, and “to screen”

- Decode target format cost (vips band formats)
- Conversion RGB→RGBA (or premultiplied) for Qt / GL
- Upload / paint proxy: pure CPU blit of a 4K frame into a buffer as a
  stand-in for “time to screen pixels” without requiring a display

**Judgement target:** whether keeping tiles RGB888 (or JPEG→RGB) and converting
late is cheaper than storing RGBA everywhere.

### 3.7 Extensions worth including in v1 design

- **Warm SQLite get_tile only** (no source I/O): pure cache hit latency.
- **Process extract LRU hit vs miss** for archive members.
- **Jobs scaling**: 1 vs N worker threads on pyramid encode (Amdahl check).
- **PDF page raster** (MuPDF): size probe vs first tile vs full page — separate
  micro-suite so image/JPEG numbers stay clean.
- **Machine fingerprint**: CPU model, nixpkgs rev, thumtoo rev, feature flags
  (`feature_unarr`, JXL, etc.) written into every result file.

---

## 4. Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  flake input: benchtoo (https://github.com/Grumbel/benchtoo)    │
│  - synthetic generators (deterministic)                     │
│  - pinned public-domain samples (JPEG/PNG/PDF/ZIP/RAR…)     │
│  - manifest.json (id, path, class, expected WxH, license)   │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│  golden-path tools (minimal C++/Python, NO thumtoo Client)  │
│  - gp-decode      (vips/libjpeg/jxl matrix)                 │
│  - gp-tile        (region → encode tile codecs)             │
│  - gp-archive     (libarchive + unarr A/B)                  │
│  - gp-pipeline    (staged time-to-pixels scripted)          │
└────────────────────────────┬────────────────────────────────┘
                             │ compare
┌────────────────────────────▼────────────────────────────────┐
│  thumtoo-instrumented path                                  │
│  - thumtoo-bench (existing; extend phases + JSON out)       │
│  - microbench_* (fold into apps or keep as tools)           │
│  - optional Client stats dump (BuildStats)                  │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│  reporter                                                   │
│  - JSONL / JSON schema per run                              │
│  - compare vs checked-in baseline (tolerance bands)         │
│  - markdown table generator for docs/MICROBENCH_RESULTS.md  │
└─────────────────────────────────────────────────────────────┘
```

### 4.1 Where code lives

| Component | Location | Rationale |
|-----------|----------|-----------|
| Corpus + generators | **New flake** `benchtoo` (or `thumtoo-bench-data`) | Large / binary; not source tree |
| Golden-path tools | Prefer **same new flake** or `thumtoo/tools/golden/` with clear “no Client” rule | Must not drag SQLite / Store |
| Library-facing benches | Stay in **thumtoo** (`thumtoo-bench`, extended) | Needs real Client API |
| Host TTFP | Stay in **biltoo** (`BILTOO_TTFP`) | Product path |
| Plan + result schema | `thumtoo/docs/BENCHMARK_KIT.md` (this file) + `docs/bench/schema.md` | Continuity |

Avoid putting multi-hundred-MB samples into either application repo.

### 4.2 Result format (sketch)

One JSON object per run (or JSONL for matrix rows):

```json
{
  "schema": 1,
  "tool": "gp-decode",
  "git": { "thumtoo": "…", "corpus": "…" },
  "nixpkgs": "…",
  "machine": { "cpu": "…", "cores": 16 },
  "case": {
    "id": "synth_8k_jpeg",
    "op": "jpegload_shrink4",
    "params": { "shrink": 4 }
  },
  "metrics": {
    "wall_ms_median": 53.1,
    "wall_ms_p95": 55.0,
    "bytes_out": 123456,
    "rss_delta_mib": 40.2
  },
  "repeats": 9,
  "warmups": 1
}
```

Baselines: checked-in under `docs/bench/baselines/<machine-class>/` or produced
by a “record baseline” mode. Comparison uses relative tolerance (e.g. fail if
median > baseline × 1.5 for the same case id) so different CPUs do not
false-fail when class matches.

---

## 5. Corpus design (`benchtoo`)

### 5.1 Synthetic (benchtoo flake)

Content-class matrix (not solid fills). Classes pick **landscape 16:9** or
**portrait ~2:3** size sets so benches match biltoo ebook/comic/album traffic:

| Class | Aspect | Purpose |
|-------|--------|---------|
| photo / landscape | landscape | Camera-like / scenic album images |
| bookpage / scan | portrait | Ebook pages (cream paper, dense text; scan + grain) |
| comic | portrait | Panel grid + speech boxes |
| spread | landscape | Two-page open book + gutter |
| text / geometry / fractal / noise / mixed | landscape | Stress edges, detail, entropy bounds |

Sizes: landscape 800×450, 1920×1080, 3840×2160; portrait 600×900, 1200×1800, 1600×2400.
Generator: `generators/gen_synthetic.py` (numpy + Pillow), seeded by (class, w, h).

Still planned: ZIP/tar/RAR archive fixtures; EXIF-thumb JPEGs; pre-encoded tile grids.

### 5.2 Public-domain / freely licensed samples (pinned)

Fetch via Nix `fetchurl` / `fetchzip` with hash pins — **not** git-lfs in app
repos. Candidates (verify license at pin time):

- Wikimedia / public-domain high-res stills (JPEG)
- Sample PDFs (e.g. well-known short public-domain documents)
- Small CBZ made from PD images if redistribution allows

Manifest lists license SPDX + source URL for REUSE-friendly attribution.

### 5.3 Size budget

Keep the default `nix build` corpus under ~200–300 MB compressed if possible;
offer an optional `corpusFull` output for heavy RAR/8K sets used only for
manual deep dives.

---

## 6. Golden paths (reference implementations)

Each golden path is intentionally dumb and linear.

### 6.1 `gp-decode`

- Input: file path + op (`size_only`, `full`, `shrink2/4/8`, `thumbnail_edge`)
- Backend: vips primary; optional libjxl / libjpeg-turbo flags
- Output: median wall, output dimensions, optional RSS

### 6.2 `gp-tile`

- Load full image once (or region-decode if API allows)
- Split into 256² cells at scale 0..K
- Encode each cell with codec × quality matrix
- Report encode total, mean per cell, total bytes, decode-back mean

### 6.3 `gp-archive`

- Open with libarchive **or** libunarr (explicit flag)
- Ops: toc, extract index i, extract all sequential, extract scattered set
- No SQLite, no thumtoo URI
- Format sniffed by magic bytes. unarr reads ZIP, TAR, RAR4 (incl. solid)
  and 7z only when libunarr was built with the 7z SDK; RAR5 is
  libarchive-only. `--backend auto` mirrors thumtoo's dispatcher (unarr for
  RAR4 only).
- `unarr-seek`: unarr with single-member extracts jumping to the entry
  offset recorded by the last TOC read (`ar_parse_entry_at`) — the
  TOC-cached Random route. Its first/last/scattered timings exclude the TOC.
  (`ar_parse_entry_for` walks by name from the start, so it is not used.)

### 6.5 Automatic comparisons ("who wins")

One command runs every variant on the same input and prints a verdict:

```bash
thumtoo-gp-archive --backend all ARCHIVE          # or --backend libarchive,unarr
thumtoo-gp-tile --codec all CORPUS/synthetic/png/photo_1920x1080.png
```

Judging (`tools/golden/gp_verdict.hpp`, unit test `gp_verdict`):

- Per metric, each variant gets a cost ratio vs the best (1.0 = best).
  Variants within `--tie-pct` (default 5%) of the best are a tie, and so is
  any variant whose fastest run is no better than the best's median
  (difference within run-to-run noise). Bytes are deterministic and decided
  by the band alone.
- gp-archive times backends interleaved (round-robin, rotating start) after
  verifying all of them; sequential timing made whichever backend ran second
  look 12–15% faster on identical code.
- Overall winner = lowest geometric mean of the per-metric ratios (equal
  weights, scale-free). Read the per-metric lines first; the overall line is
  a summary, e.g. JPEG "wins" tiles overall on encode speed while losing
  on bytes.
- Only **verified** variants are ranked. A failed or unsupported variant is
  listed with its reason; it can never win with zero bytes / zero ms.

Archive specifics: before timing, each backend's extract-all bytes must
equal its TOC sizes and the first/last member sizes must match; in
comparison mode all backends must agree on member count and byte total,
otherwise the verdict is refused (exit 1). Damaged archive → `failed`;
format a backend cannot read → `unsupported`.

Codec specifics: codecs are **not** compared at equal `Q` (the scales are
unrelated). The PSNR of `--reference` (default `jpeg:80` =
`kDefaultTileCodec`/`kDefaultTileQuality`) is the target; each codec is
represented by its smallest output reaching it, refined by bisecting `Q`
between sweep points, and judged on bytes_total / encode_ms / decode_ms.
Encoder effort is part of the variant: `webp@e0..6`, `avif@e0..9`,
`jxl@e1..9` (e.g. `--codec all,webp@e0,jxl@e1,jxl@e3`); bare names use the
libvips defaults (webp 4, avif 4, jxl 7), which are slow for interactive use.
Use a **lossless** source: on a q90 JPEG, re-encoding JPEG at q90 is nearly
lossless (~79 dB) and biases everything; the tool warns and records
`source_lossy` in JSON. PSNR is a crude proxy (see follow-ups in TODO.md).

JSON: comparison documents carry `"kind": "compare"`, per-variant results
(`variants[]` / `codecs[]` + `matched[]`) and `verdict{metrics, overall}`.
Single-variant output keeps its schema-1 keys, so existing baselines
compare unchanged; effort variants add `variant`/`effort` to their rows.
`compare_bench_json` keys list elements by identity and reports verdict
winner changes; `--fail-on-winner-change` gates only on the **overall** (or
aggregate) winner, because per-metric winners flip between identical runs
on near-tie metrics.

### 6.6 Command line and output formats (all bench tools)

`thumtoo-gp-archive`, `thumtoo-gp-tile`, `thumtoo-microbench-decode` and
`thumtoo-bench` share one command-line layer (`tools/golden/gp_cli.hpp`,
unit test `gp_cli`) and one output layer (`gp_output.hpp`, test `gp_output`).
`--help` lists every option, the output formats, exit status and examples;
`--version` prints the thumtoo version.

**Inputs.** The golden tools take any number of files and directories.
Directories are searched by *content* (archive magic bytes; libvips loader
detection; JPEG) and sorted by name; `-r/--recursive` descends. A file named
explicitly is always attempted. Duplicates are dropped.

**Validation.** Everything is checked before anything is measured and all
problems are reported together (exit 2): unknown options with a "did you
mean" hint, non-numeric or out-of-range values, unknown or unavailable
backends/codecs, `--csv` with `--json`, missing paths.

**Output.** One format per run; stdout carries only that format, warnings
and errors go to stderr.

| Mode | For | Shape |
|---|---|---|
| default | reading | aligned tables with units, sizes as KiB/MiB, verdict in words; with several inputs and a comparison a summary (wins/ties and geometric-mean cost per variant over all inputs; inputs with fewer than two measurable variants are listed as skipped) |
| `--csv` | spreadsheets, `>>` appends | one flat table, raw ms/bytes, RFC 4180 quoting, empty cell = not measured, `--no-header` to append |
| `--json` | tools, baselines | schema 1; one input = the document as before (+ additive `status`); several inputs = `{"kind":"batch","results":[...],"aggregate":{...}}` |

CSV columns (stable names):

- gp-archive: `archive,format,backend,status,reason,members,bytes,toc_ms,extract_all_ms,extract_first_ms,extract_last_ms,extract_scattered10_ms,overall_ratio` — one row per archive × backend.
- gp-tile: `file,width,height,variant,codec,effort,status,reason,quality,encode_ms,decode_ms,bytes_total,bytes_per_cell_mean,psnr_db,matched,overall_ratio` — one row per image × variant × quality; `matched=1` marks the setting that represents the variant in the verdict.
- microbench-decode: `file,width,height,mpix,status,reason,size_ms,full_ms,shrink2_ms,shrink4_ms,shrink8_ms,thumb32_ms,thumb256_ms`.
- thumtoo-bench: `phase,wall_s,workers,images` + the BuildStats counters, one row per phase plus `total`.

`overall_ratio` is the cost relative to the best variant (1 = best) and is
empty when only one variant could be measured.

**Failures are in-band.** An archive/image that cannot be read, or a variant
that cannot run, appears with `status` `failed`/`unsupported` and a `reason`
in every format; the rest of the batch still runs; the exit status is 1 and a
one-line summary goes to stderr. Exit codes: 0 ok, 1 measurement failed (or
no verdict), 2 usage error.

**Silent-failure fixes** that came with this: libunarr/libvips diagnostics
are silenced while measuring (`gp::StderrSilencer`; they repeat on every timed
call); `microbench-decode` now verifies each operation and rejects non-JPEG
and truncated files instead of timing a failed decode as ~0 ms.

End-to-end test: `tests/test_bench_tools_cli.py` (ctest `bench_tools_cli`).

### 6.4 `gp-pipeline`

Scripted stages matching §3.5 on a single file or one archive member:

```
size → (embedded?) → extract? → shrink|full → (optional tile encode) → RGB buffer
```

Print stage table; this is the **yardstick** for `thumtoo-bench` cold path.

### 6.5 Host-side (biltoo, separate)

Keep `BILTOO_TTFP` as the product golden path (Gallery open → first pixels).
Do not reimplement GUI timing inside the corpus flake.

---

## 7. Library path extensions

### 7.1 `thumtoo-bench`

- Add **JSON output** (`--json`) matching the schema.
- Add phases: warm `get_tile` after cold pyramid; optional codec override if
  API allows (else document that tile codec is compile/schema default).
- Keep cache wipe for cold runs; add `--keep-cache` for warm.

### 7.2 New or extended microbenches in-tree

Fold `microbench_decode` into a maintained `thumtoo-micro-decode` tool linked
against the same vips/jxl as the library (not Pillow), so numbers match
production.

Archive microbench should call the same functions as `archive.cpp` /
`archive_unarr.cpp` (or thin wrappers) so “libarchive vs unarr” is apples-to-apples.

### 7.3 Debug flags / statistics (not always a separate suite)

- Ensure `BuildStats` (or successor) can dump per-phase ns after a bench run.
- Optional env `THUMTOO_BENCH_TRACE=1` for stage logs without full bench binary.
- biltoo already has TTFP; optionally emit JSON there too for the reporter.

---

## 8. Nix integration

### 8.1 thumtoo flake

- `apps.bench` → existing / extended `thumtoo-bench`
- `apps.micro-decode`, `apps.micro-archive` (or one `apps.bench-matrix`)
- `checks.bench-smoke`: short run on synthetic 2 MP JPEG + small ZIP from
  corpus input; compare against loose baseline or “finishes & prints JSON”
- Document `nix run .#bench -- /path --tiles --json`

### 8.2 Corpus flake

- `packages.corpus` / `packages.corpusFull`
- `apps.generate` for synthetic rebuild
- Expose path via `THUMTOO_BENCH_CORPUS` or flake input path for thumtoo checks

### 8.3 biltoo

- Optional check that `BILTOO_TTFP=1` smoke open against a tiny fixture
  completes (heavier; may stay manual)

---

## 9. Phased roadmap

| Phase | Deliverable | Depends |
|-------|-------------|---------|
| **P0** | This plan + inventory; decide repo name for corpus flake | — |
| **P1** | Corpus flake skeleton: synthetic JPEG/PNG generators + manifest; wire as thumtoo flake input | P0 |
| **P2** | Golden `gp-decode` + `gp-tile` (codec matrix) + JSON schema; document first results in MICROBENCH_RESULTS | P1 |
| **P3** | Golden `gp-archive` (libarchive + unarr) + sequential vs random cases | P1 |
| **P4** | `gp-pipeline` stage table; extend `thumtoo-bench` JSON + warm tile phase; side-by-side overhead report | P2 |
| **P5** | RGB vs RGBA + “to buffer” blit proxy; jobs scaling curve | P2 |
| **P6** | `checks.bench-smoke` + baseline compare mode; refresh PERFORMANCE.md pointers | P4 |
| **P7** | Optional PDF micro-suite; optional public-domain pin set | P1 |

Each phase should land as normal thumtoo (or corpus) commits with bundles per
standing agent rules; keep commits task-focused (generators ≠ harness ≠ docs).

---

## 10. Open questions (resolve before or during P1–P2)

1. **Corpus repo home:** under Grumbel/ on GitHub, or private until stable?
2. **AVIF encode in matrix:** worth the dependency weight if encode is never
   on the interactive path? (Decode-only AVIF may still be useful.)
3. **Tile codec configurability in Store:** today default is JPEG; matrix may
   need a bench-only encode API that does not change production defaults.
4. **Baseline machine class:** single reference (e.g. “nixx86-zen3”) vs
   multiple? Start with one class + relative tolerances.
5. **Solid RAR fixtures:** generating solid RAR in Nix may need `rar` (non-free)
   or vendoring a tiny fixed binary fixture with clear license.
6. **How much of golden tools share thumtoo’s `image.cpp` helpers?** Prefer
   zero linkage for true overhead measurement; duplicated thin vips wrappers
   are acceptable.

---

## 11. Success criteria

- A developer can run one command and get a **codec × quality** table for tile
  encode/decode on the synthetic corpus.
- A developer can compare **libarchive vs libunarr** on the same RAR4 solid
  fixture with median ms for TOC and extract patterns.
- `thumtoo-bench` JSON for cold path can be placed next to `gp-pipeline` JSON
  for the same file; overhead is visible stage-by-stage.
- `nix flake check` on thumtoo runs a **smoke** subset and fails on catastrophic
  regression (order-of-magnitude), not on 5% noise.
- PERFORMANCE.md / MICROBENCH_RESULTS.md cite the kit instead of one-off
  `/tmp` experiments.

---

## 12. Suggested first implementation slice (after plan approval)

1. Create `benchtoo` flake with synthetic JPEG generator (0.5–8 MP)
   and empty manifest schema.
2. Add `tools/golden/gp_decode.cpp` (or corpus-side) using vips only; flake app.
3. Extend `thumtoo-bench` with `--json`.
4. Write one checked-in result table replacing the Pillow-era JPEG section in
   MICROBENCH_RESULTS.md.

Stop and review numbers before building the full codec × archive matrix.
