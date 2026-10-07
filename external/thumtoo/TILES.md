<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Display pixels: grid tiles (durable) vs whole-image helpers

thumtoo exposes **one durable multi-res pixel path** (grid tiles) plus
ephemeral whole-image helpers. Normative policy:
[docs/PIXEL_AND_ARCHIVE_POLICY.md](docs/PIXEL_AND_ARCHIVE_POLICY.md).

| Path | API | Durable? | Typical use |
|------|-----|----------|-------------|
| **Grid tiles** | `get_tile` / `request_tile` | **Yes** | Deep zoom, Gallery/Image display, Galapix |
| **LQIP** | `get_lqip` / `request_lqip` | **Yes** | Tiny placeholder |
| **Whole-image soft / overview** | `get_pixels` / `request_pixels` | **No** (ephemeral or TileSynth) | Filmstrip cold path; PreferCache plateau |

## Soft / whole-image rules (ephemeral)

1. **Not stored.** Store schema ≥ 100 does not persist soft ladder levels.
   `get_pixels` returns TileSynth when a complete tile scale exists, else miss.
2. **`request_pixels`** may still generate a ≤ `kMaxSoftLadderEdge` (512) raster
   in the worker and return it once (RAM / host ImageCache only).
3. **Prefer tiles.** Hosts that can paint cells (biltoo) should schedule
   `request_tile` rather than relying on soft replies.
4. Consumers that need more than a soft edge on screen should call
   **`request_tile`** or a full source decode — not larger soft levels.


## Grid tiles (Phase 4)

Optional **galapix-compatible** tile pyramid.
Tiles are the durable multi-res path; soft/overview is ephemeral or TileSynth.

## Model (Galapix-aligned)

| Field | Meaning |
|-------|---------|
| `scale` | 0 = full resolution; each +1 halves width and height |
| `x`, `y` | Tile indices from the top-left of that scale’s image |
| Tile size | **256×256** (`kTileSize`); edge tiles may be smaller |
| Codec | Default **JPEG** q=80 (`kDefaultTileCodec` / `kDefaultTileQuality`) |
| Source cap | Encode refused when `width*height > kTileMaxSourcePixels` (100 MP) |

Whole-image coverage at scale `s` is:

```text
tw = ceil(width  / 2^s / 256)
th = ceil(height / 2^s / 256)
```

`max_scale` is the smallest `s` where `tw == 1 && th == 1` (or a configured cap).

## Storage

- **index.sqlite** `tiles(content_id, scale, x, y, width, height, codec, quality, source)`
  (`source`: 0=full, 1=jpeg_shrink, 3=pdf_region, 4=djvu_region — see `TileSource`)
- **blobs.sqlite** `tile_blobs(... same key ..., data BLOB)`
- Content identity is still `sha256:…` / provisional; locators unchanged.

Additive schema only; `schema_version` stays 1 until a breaking migration.

## API (Client)

```text
get_tile(uri, scale, x, y) -> optional<TileBlob>
request_tile(uri, scale, x, y, callback)
get_tile_coverage(uri) -> optional{min_scale, max_scale, Size}
request_tile_pyramid(uri, min_scale, max_scale, callback)  // optional batch
```

`TileBlob`: scale, x, y, width, height, codec, bytes.

On miss, interactive `request_tile(s)` encode **one cell** (not the full pyramid).
JPEG `scale>0` uses DCT `jpegload` shrink; concurrent cells of the same file share
one shrink decode (`jpeg_shrink_acquire`, tip 305). Pyramid prepare still builds
all scales offline.

Historical note: an older path built the requested scale and coarser scales in one
pass on miss; interactive path is single-cell. Finer scales than already stored are generated only when
asked. Sources larger than `kTileMaxSourcePixels` yield no tiles (probe/size
still work).

## Interactive cell contract (`request_tile_cells`)

Normative for viewers (biltoo tile LOD). `request_tile_cells(uri, coords, cb)`
delivers **exactly one `TileResult` per index**, always via the Executor,
streamed per cell as each one finishes (not after the whole batch):

| `TileStatus` | Meaning | Host action |
|--------------|---------|-------------|
| `Ok` | `tile` holds rgb888/rgba8 pixels | Paint |
| `Cancelled` | Host cancel (`cancel_tile_cells`, `cancel_uri`, `cancel_pending`) or shutdown before the cell was produced | Forget; re-request if still wanted |
| `Failed` | I/O, decode, render or internal error; `error` says why | Show the reason; retry with backoff |
| `Unavailable` | The cell cannot exist (outside the grid, denser scale on a raster image, PDF raster page finer than its native dpi) | Show the reason; never retry |

Rules:

- **Never silence.** Cancel and shutdown deliver `Cancelled`; worker exceptions
  deliver `Failed` with the exception text; a backstop after each batch answers
  any cell the batch body missed.
- Cancel marks cells and never compacts a batch, so index → coordinate mapping
  stays stable. A cell that is already being computed may still deliver `Ok`.
- Cell jobs are **exempt from `bump_interest_epoch` / `set_interest`**: the host
  owns their lifetime via `cancel_tile_cells`. (Silently dropping queued tile
  jobs on every Gallery scroll left biltoo cells InFlight forever.)
- No same-cell supersede and no duplicate merging: the host must keep at most
  one outstanding request per `(uri, scale, x, y)`.
- Cell jobs are FIFO among themselves (the host submits coarse cells first);
  the worker claim loop still prefers interactive tiles over bulk work.

Legacy `request_tile` / `request_tiles` keep the old contract (`nullopt` for
misses, **no callback** for cancelled jobs). `request_tiles` is a thin wrapper
over `request_tile_cells`.

Test: `tests/test_tile_cells_contract.cpp`.

## Prepare

`thumtoo-prepare --tiles [paths…]` prewarms full pyramids (subject to source
size policy). Default prepare path does **not** build tiles.

## Non-goals

- GUI / OpenGL cache (stays in Galapix)
- Video or PDF page tiles in the first cut
- Replacing the JXL ladder

## Integration

See [INTEGRATION.md](INTEGRATION.md) for biltoo. Galapix mapping will live in
`INTEGRATION_GALAPIX.md` once the Client tile API is stable: replace
`SQLiteTileDatabase` / `TileGenerator` with thumtoo `get_tile` / `request_tile`
keyed by URL → content_id.

## Performance notes (2026-09-07)

* Interactive `request_tile`: **one scale only** (`tile_max_scale = scale`). Use
  `request_tile_pyramid` / `thumtoo-prepare --tiles` for full pyramids.
* `prepare --tiles --stats --jobs N`: wall time vs CPU-share (jpeg usually
  dominates after extract cache).
* Archive members: extract once per batch when possible; bytes may live briefly
  in the in-process extract cache (max 512 MiB).
* Encoding: parallel JPEG for cells within a scale; Client runs multiple jobs
  across URIs concurrently.

## Interactive vs prepare

* `request_tile(uri, scale, x, y)` encodes **only that cell** (load → shrink to
  scale → crop → JPEG). Does not fill the rest of the scale grid.
* `request_tile_pyramid` / `thumtoo-prepare --tiles` still build full scale
  ranges for offline prewarm.

### JPEG load for interactive cells

**Intended:** For JPEG sources, `build_tile_cell_buffer` can use
`vips_jpegload(..., shrink=2|4|8)` so coarse scales avoid a full-resolution
decode, then apply remaining factor-of-two shrinks.

**Current (2026-09-09, Option A):** Interactive `build_tile_cell` /
`build_tile_cell_buffer` use `vips_jpegload(..., shrink=2|4|8)` for
**scale > 0** JPEG sources (file path and buffer), independent of
`decode_cache_key`. Stored tiles record `source = JpegShrink`. Scale 0 and
non-JPEG still full-load; multi-cell scale 0 can use the in-process shrink
ladder after a full decode.

## Scale range (detail cutoff)

Galapix scale **0 = full resolution**; higher = coarser.

* `request_tile_pyramid(uri, min_scale, max_scale)`  
  - `min_scale`: finest scale to store (0 = full res; `1` skips full-res tiles)  
  - `max_scale < 0`: generate until the image fits in one 256² tile  
* CLI: `thumtoo-prepare --tiles --min-scale N --max-scale M`  
  (`--min-scale` / `--max-scale` imply `--tiles`)  
* Interactive `request_tile` still encodes only the requested cell; the app
  chooses which scales to ask for.


## PDF layout DPI + region tiles (2026-09-07)

Page `get_size` uses **kPdfLayoutDpi (144)** (media box × 144/72) as the
**nominal** full-resolution size for layout. Do not change this constant
without migrating or discarding existing tile rows.

### Scale geometry (must stay consistent with Galapix)

Let `L` = layout size at 144 dpi. Tile size `T = 256`.

| scale `s` | full pixel grid | dpi | tile `(x,y)` covers layout px |
|----------:|-----------------|-----|-------------------------------|
| 0 | `L` | 144 | `[xT,(x+1)T) × [yT,(y+1)T)` of `L` |
| +1 | `L/2` | 72 | 2× coarser |
| −1 | `2L` | 288 | half the linear span of a scale-0 tile |

`full = pdf_page_size_at_scale(L, s)`:
- `s >= 0`: successive floor-half (`dim_at_tile_scale`) — matches image pyramid
- `s < 0`: exact `L * 2^{-s}` (integer)

`dpi = kPdfLayoutDpi * 2^{-s}`

### PDF rendering (2026-10-07)

Every PDF cell is a region render of the page's **display list** with the
cell as the device rect, at any scale. There is no full-page raster and no
page-size limit (a Letter page at −4 is 496 MP; one cell is still 256²).

Runtime (`src/pdf_mupdf.cpp`):

- One base `fz_context` with locks owns the **shared store** (decoded images,
  glyphs; `kPdfStoreBytes`). Each thread renders with an `fz_clone_context`.
- One `fz_document` per file (`DocEntry`, LRU `kPdfDocumentCacheSize`), used
  under a mutex — MuPDF documents are single-threaded. Waits are counted
  (`PdfDocumentRenderStats::lock_waits`).
- Each page (LRU `kPdfPageCacheSize`) keeps a display list built **once**
  through a profiling pass-through device. Cells run that list outside the
  document lock, concurrently.
- The profiling device records `PdfPageProfile` (below) and wraps every image
  in a `CountingImage`.

**Decode once.** `CountingImage` decodes the whole image (at the requested
subsample level) when it fits `kPdfFullImageDecodeBudget`, so the store keys
it as the full image and every later cell hits it. Each real decode is
counted in `PdfDecodeStats` (full / subarea, pixels, ms). Images over the
budget fall back to MuPDF's per-cell subarea decode, counted with a reason.
Decodes are **single-flight** per image: cells on other threads that need the
same decode wait for it (`shared_waits`) instead of decoding again, so a page
costs one decode per image and subsample level regardless of worker count.

Why whole-image decode matters: MuPDF re-derives the image matrix for every
subarea (`update_ctm_for_subarea`), so per-cell subarea decodes resample with
a slightly different phase and neighbouring cells disagree (measured: up to
209/255 on a high-frequency scan). That was the "region draws seam on scans"
the old full-page-raster path worked around. With one decode, stitched cells
are byte-identical to one render (`tests/test_pdf_profile.cpp`). Diagonal
strokes differ by a few levels on anti-aliased edge pixels between any two
clip regions — MuPDF clips stroke segments to the scissor in floating point
(`draw-path.c`); the test allows that explicitly.

**Page profile** (`pdf_page_profile`): kind = Empty / Vector / Raster /
Mixed, from what the page actually draws (contents, annotations, widgets):

| Observed | Counts as |
|----------|-----------|
| fill/stroke paths, shadings, visible glyphs, clip glyphs | vector detail |
| `fill_image`, `fill_image_mask`, `clip_image_mask` | image (dpi from the ctm) |
| invisible text (render mode 3, OCR layers) | ignored (`invisible_glyphs`) |
| first fill covering ≥ `kPdfBackgroundFillCoverage` of the page | paper background, ignored |

Only **Raster** pages get a resolution cap: `finest_useful_scale` is the
coarsest scale whose dpi reaches `native_dpi / kPdfNativeDpiTolerance`
(300 dpi scan → −1, 600 dpi → −2). Finer cells answer `Unavailable` with the
reason. Vector and Mixed pages are never capped (a stamp on a scan stays
sharp). `PdfPageProfile::summary` states the decision in one line; hosts show
it.

Inspect any file: `thumtoo-page-profile [--json] [--render SCALE|cap]
[--threads N] FILE [PAGE…]` (PDF, DjVu, EPUB) — profile plus a decode-count
benchmark.

### EPUB rendering (2026-10-07)

EPUB runs on the same runtime (`src/mupdf_runtime.hpp`): the layout (page
box, base font, reader-policy CSS) is an `OpenSpec`, and each (file, layout)
is one cached document, laid out **once per process** (it used to be laid out
once per worker thread and again on every book/layout switch). Pages get
display lists, profiles (text pages Vector, a full-page cover Raster with its
CSS page background ignored, figures Mixed), decode-once images, the same
cell status/reason contract and render stats (`opens` counts layouts).
Internal links and outline entries resolve to absolute pages
(`fz_page_number_from_location`; a location is chapter + page-in-chapter, so
every EPUB outline entry used to point at page 1).
`tests/test_epub_tiles.cpp` on `tests/fixtures/book.epub`
(`make_epub_fixture.py`).

### DjVu rendering (2026-10-07)

`src/djvu.cpp`. Layout (scale 0) is the page's **native pixel grid**; coarser
scales floor-halve like the host grid (`dim_at_tile_scale`). A DjVu page is a
raster, so scales < 0 answer `Unavailable` ("finer than the page's native
pixels") instead of interpolating.

- One cached document per file (LRU 4), each with its own ddjvu context and
  mutex: books render in parallel; one book serializes (ddjvu contexts are
  single-threaded). `ddjvu_context_create` itself is serialized globally
  (it calls `setlocale`).
- Decoded pages stay alive (LRU 4 per document): every cell of a page renders
  from **one decode** (was: create + decode + `ddjvu_cache_clear` per cell —
  13–25 ms per cell on IW44 pages, now 0.2–2 ms).
- `djvu_page_profile` (same `PdfPageProfile` type): Raster with the JB2 mask
  and IW44 layers and their dpi (sizes from `ddjvu_document_get_pagedump`),
  hidden text layer size as `invisible_glyphs`, `finest_useful_scale` 0;
  Empty for pages without image layers (those render white — the only case
  where a FALSE `ddjvu_page_render` is not an error).
- Decoder errors (ddjvu error messages) are kept and returned as the cell's
  reason; `djvu_document_render_stats` counts page decodes, cells, refusals,
  failures and lock waits.
- Text layers: every word used to be listed twice (line children walked
  twice) — fixed; `test_djvu_text` now runs on the committed fixture.
- Stitching: JB2 is exact; IW44 layers differ by ≤ 6/255 between render rects
  (djvulibre reconstructs wavelets per rect; removing it needs a ~128 px margin
  per cell — not worth 3× the pixels). `tests/test_djvu_tiles.cpp` on
  `tests/fixtures/pages.djvu` (regenerate: `make_djvu_fixtures.sh`).
- Known: TSan reports djvulibre's decoder thread unlocking a page monitor
  that is destroyed later on eviction (inside djvulibre, no visible edge).

**Layout pixels:** one `lround(page_pt * dpi/72)` from the continuous page
bound (`fz_bound_page`). Do not round to integer 72dpi points and scale again
(that drifts by up to 1 device pixel vs the region ctm).

Each cell is ≤ `T²` pixels. Durable cache keys `(content_id, scale, x, y)`.
PDF cells finer than `kPdfMinDurableTileScale` (−2, 576 dpi) are generated
live and **not** stored. Interactive PDF `request_tile` replies with
**`codec=rgb888` raw pixels** (no JPEG). Durable store (scale ≥
`kPdfMinDurableTileScale`) still writes JPEG at `kPdfTileQuality` for the
next cache hit.

Raster images never use negative scale.

## Full-frame construct from tiles

When a host needs a **whole-image** raster at an arbitrary long edge (not a
single deep-zoom cell), and a grid pyramid is already warm:

```text
get_pixels_from_tiles(uri, max_edge)  // cache-only
```

1. Pick pyramid scale whose long edge still covers `max_edge` (else coarsest).
2. Require **every** tile at that scale (`get_tile`); incomplete → miss.
3. Composite to RGB, shrink to `max_edge`, encode JXL.
4. `PixelSource::TileSynth`.

`get_pixels` prefers this path when `allow_tile_synth` is true and a
complete scale exists. Soft is not durable; mid-edge UI (≤ `kBatchMaxEdge`
1024) should use TileSynth or tiles rather than a soft ladder.

See biltoo `docs/PIXEL_PIPELINE_REDESIGN.md` for batch vs focus lanes that
*build* the pyramid; this API only **reads** it.

## Archive backends (RAR / solid)

| Backend | Formats | Solid RAR | RAR5 |
|---------|---------|-----------|------|
| **libarchive** (default) | zip, 7z, rar, … | Often **unsupported** (`RAR solid archive support unavailable`) | Partial |
| **libunarr** (optional) | **RAR / CBR** focus | **Yes** (sequential walk) | **No** (upstream WIP) |

When built with `THUMTOO_HAVE_UNARR`, `.rar` / `.cbr` use unarr for TOC + extract
(`//archive:` URIs unchanged). Extract does **not** fall back to libarchive for
RAR — libarchive cannot extract solid RAR and only confuses RAR5 failures.
**RAR5:** libunarr cannot open it (spam on stderr if called). thumtoo probes the
magic and routes RAR5 to **libarchive** (TOC + extract for non-solid). Solid RAR5
remains limited. RAR4 solid stays on unarr. ZIP/CBZ stay on libarchive.

**Seek:** unarr can `ar_seek` the **file stream** and `ar_parse_entry_at` **headers**.
Solid **payloads** are not randomly seekable — extract walks members in order and
must uncompress intermediate solid members to keep the dictionary (see
`extract_archive_members_unarr`). Prefer sequential batch windows for CBR.

## Activity ledger (tile jobs)

`Client::activity_snapshot()` exposes `tile_queued` / `tile_running` for host
status (e.g. biltoo Performance panel).

| Event | Ledger |
|-------|--------|
| `request_tile` / batch / pyramid enqueue | `note_tile_queued` |
| Worker starts cell | `note_tile_running` |
| Worker done / miss | `note_tile_finished` |
| **Supersede** same `uri/scale/x/y` still queued | `reply_cancelled_job` → `note_tile_finished` |
| Pyramid supersede / cancel / interest drop | same `reply_cancelled_job` |

**Invariant:** dropping a queued `EnsureTiles` job without `note_tile_finished`
leaks `tile_queued`. Hosts then show **Working** with `tile=N/0` while
`queue_stats().pending` is 0.

Regression tests: `tests/test_activity.cpp` (finish without Running),
`tests/test_tile_supersede_activity.cpp` (Client single-cell flood).
