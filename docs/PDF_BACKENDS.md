# PDF backend (MuPDF)

PDF pages use **MuPDF** only (`THUMTOO_HAVE_MUPDF`). Poppler was removed.

## URI routes

| Pipe | Meaning |
|------|---------|
| `//page:N` | Default page (MuPDF) |
| `//mupdf-page:N` | Explicit MuPDF |
| `//poppler-page:N` | Accepted for old paths; normalized to `PdfPage` / `//page:N` on format |
| `//pdfimage:N` | Embedded Image XObject extract (1-based, native resolution) |
| `//pdfimages` | Collection expand → `//pdfimage:1..N` |

```
file:///data/doc.pdf//page:1
file:///data/doc.pdf//mupdf-page:1
file:///book.pdf//pdfimage:3
file:///book.pdf//pdfimages
```

`prepare_paths` / expand emit `//page:N` by default for PDFs.

## Layout

| File | Role |
|------|------|
| `pdf.hpp` | Types, URI, scale math, public API |
| `pdf.cpp` | URI parse/build + dispatch to MuPDF |
| `pdf_mupdf.cpp` | MuPDF implementation |

`PdfBackend::Poppler` remains as a deprecated ABI alias of `Default`.

## Capabilities

- Page raster / region tiles (display list + device-space scissor)
- Image-heavy gate via `fz_stext` image blocks
- Text layer + outline (Find / ToC)
- Embedded images: `//pdfimage:N`

## Embedded images

```
file:///book.pdf//pdfimage:1
file:///book.pdf//pdfimages
```

Expand helpers: `expand_pdf_image_uris` / `expand_pdf_images_collection_uri`.
Default expand for PDFs uses rendered `//page:N`.

### Client plumbing (Store path)

`//pdfimage:N` is a first-class locator, not a page render:

| Job | Behaviour |
|-----|-----------|
| ProbeSize | `pdf_embedded_image_size` (dict `/Width`/`/Height` when possible) |
| EnsurePixels | `pdf_rasterize_embedded_image` → soft ladder |
| EnsureTiles | full extract → `build_tile_cell_rgb` / pyramid |

Store keys: content_id `sha256:…:pdfimage:N`, Document media, `RegionKind::Fragment` key `N`.

## MuPDF version (flake)

nixpkgs ships MuPDF 1.27.x. thumtoo pins **1.28.5** via `pinMupdf` in
`flake.nix` (exported through `lib.mkBuildInputs`) for Markdown document
support. Rebuild if PDF/EPUB regressions appear after the pin.

## Markdown

`.md` files use the same MuPDF `//page:N` path as PDF when MuPDF ≥ 1.28 is
linked. See [MARKDOWN.md](MARKDOWN.md).
