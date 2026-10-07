<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# EPUB (MuPDF reflowable layout)

## Why MuPDF

thumtoo already links MuPDF for PDF. MuPDF also opens EPUB (and FB2/HTML)
as reflowable documents via `fz_layout_document(ctx, doc, w, h, em)` then the
same page count / bound / raster path as fixed-layout docs.

CREngine remains a possible later backend if typography quality is insufficient.

## Layout is part of identity

Unlike PDF, page index and pixel size depend on:

| Parameter | Unit | Role |
|-----------|------|------|
| `w` | **pixels** at `kEpubLayoutDpi` | Virtual page width |
| `h` | **pixels** at `kEpubLayoutDpi` | Virtual page height |
| `fs` | points | MuPDF default font size (`fz_layout_document` em arg) |
| `mt`/`mr`/`mb`/`ml` | **pixels** at `kEpubLayoutDpi` | Top/right/bottom/left margin (optional) |
| `lh` | int percent | Line height ×100 (`140` → 1.4); default **140** |
| `cols` | int 1–6 | CSS `column-count` (omit when 1) |
| `cgap` | pixels @ layout DPI | CSS `column-gap` when `cols` > 1 |
| `align` | token | `publisher` / `left` / `right` / `center` / `justify` |
| `ff` | token | `publisher` (default) / `serif` / `sans` / `mono` |
| `theme` | token | `day` (default) / `sepia` / `night` |
| `pubcss` | 0 or 1 | `1` (default) use publication CSS; `0` ignore it |

`w`/`h`/margins are converted to points when calling MuPDF:
`pt = px * 72 / kEpubLayoutDpi`.

Reader policy (not part of the EPUB format) is applied via MuPDF user CSS and
`fz_set_use_document_css` before layout: forced `fs`, optional margins /
line-height / font-family / theme colours. The old `em` URI key was dropped.

Optional later: custom CSS blob (`css=sha256:…`).

**Content id** for the file stays `sha256` of the `.epub` bytes.  
**Page content id** (tiles / size target) is  
`sha256:…:page:N:epub:{format_epub_layout_params}` so layouts do not share tiles.  
**Region key** is `{N}|{layout_key}` (same layout string).  
**Locator URI** still includes the full `//epub:…//page:N` pipe.

## URI

```
file:///books/foo.epub//epub:w=900,h=1350,fs=15//page:3
```

| Pipe | Meaning |
|------|---------|
| `//epub:w=…,h=…,fs=…[,mt=…,mr=…,mb=…,ml=…]` | Layout profile (canonical emit order `w,h,fs` then margins if any; unknown keys ignored) |
| `//page:N` | 1-based page **after** that layout |

Helpers: `with_epub_layout(base, layout)`, `epub_page_uri(path, page, layout)`.

### Defaults (`constants.hpp`)

| Constant | Value |
|----------|-------|
| `kEpubDefaultPageWidthPx` | 900 |
| `kEpubDefaultPageHeightPx` | 1350 |
| `kEpubDefaultFontSizePt` | 15 |
| `kEpubLayoutDpi` | 144 |

`expand_media_uris` emits pages under the default profile only.

### Cache-key stability

`format_epub_layout_params` always emits keys in fixed order (`w,h,fs` then `mt,mr,mb,ml` when any margin is set).
`parse_location` and `format_location` rewrite `//epub:` payloads through
`format_epub_layout_params`, so key order does not split the tile cache.
Hand-written URIs with a different key order still parse the same values;
full
normalization on parse is a follow-up (see TODO).

## API surface (`epub.hpp`)

- `is_epub_path` / `parse_epub_uri` / `epub_page_uri`
- `epub_page_count(path, layout)` — layouts then counts
- `epub_page_layout_size(path, page, layout)` — pixels at `kEpubLayoutDpi`
- `epub_render_tile_cell` / region raster (MuPDF, device-space scissor)

## Client

Size probe and live tiles treat `//epub:…//page:N` like PDF pages (rgb888 live,
durable JPEG at `scale >= kPdfMinDurableTileScale`).

## Non-goals (v1)

- User CSS / themes in URI
- Continuous scroll (app maps to pages)
- DRM
- Dual backend (CREngine)
