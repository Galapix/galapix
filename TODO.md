# TODO / agent handoff

## Status (2026-10-07)

**Tip:** PDF rendering rewrite — shared display lists, decode-once images,
page profiles (Claude Code, direct commit on master).

### PDF rendering rewrite
Shared MuPDF runtime (one store, cloned contexts), one document per file,
display list + `PdfPageProfile` per page, `CountingImage` decode-once with
exact `PdfDecodeStats`, per-cell region renders at any scale (full-page raster
and its 100 MP limit removed; broken stext "image-heavy" heuristic removed).
Raster pages cap at their native dpi (`Unavailable` + reason below the cap).
API: `pdf_page_profile`, `pdf_scale_refusal`, `pdf_document_render_stats`,
`pdf_render_tile_cell` → `PdfCellRender{status, raster, error}`.
Docs: TILES.md "PDF rendering". Tests: `tests/test_pdf_profile.cpp` (fixtures
built with MuPDF's writer, `tests/pdf_fixtures.cpp`). Tool:
`thumtoo-page-profile`. Corpus: benchtoo `gen_pdf_classes.py`.

### DjVu rendering
Per-document cache + contexts, decoded pages reused across cells (6–17×
faster), host-matching coarse grid, page profile + render stats, real error
reasons, scales < 0 Unavailable. TILES.md "DjVu rendering"; test
`test_djvu_tiles` with committed `tests/fixtures/pages.djvu`.

### EPUB rendering
On the shared MuPDF runtime (OpenSpec = layout): one layout per (file,
layout) per process, display lists, profiles, decode-once, status/reason,
stats. Outline/link pages fixed (were chapter-relative). Test
`test_epub_tiles` with committed `tests/fixtures/book.epub`.
Tool renamed `thumtoo-page-profile` (PDF, DjVu, EPUB).

### request_tile_cells
Interactive cells now answer exactly once each (Ok / Cancelled / Failed /
Unavailable + reason), streamed per cell, exempt from interest-epoch purges,
cancellable inside batches. Contract: TILES.md "Interactive cell contract";
test `tests/test_tile_cells_contract.cpp`. Legacy `request_tile(s)` unchanged.
biltoo's tile loader (docs/TILE_STATE_MACHINE.md) requires this API.

### Earlier: 040.2

### 040.2 — Smooth image scaling for denser PDF tiles
`set_smooth_image_scaling(bool)` / `smooth_image_scaling()` process-wide.
MuPDF `fz_tune_image_scale` uses Mitchell for upscales when smooth (default
on). (The page-level TLS raster cache this keyed on was removed in the PDF
rendering rewrite; the filter applies at draw time.)

Hosts (biltoo View → Smooth Scaling) should call `set_smooth_image_scaling`
and invalidate live denser tile RAM.

### 040.1 — Image-heavy denser via full-page crop
(see prior commit)

### Bundle policy
Cumulative `.bundle`; no parallel histories.
