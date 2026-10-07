<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Embedded preview (Q0 underlay)

Fast first-paint pixels from **container metadata**, not from content-derived
tiles or ThumbHash LQIP.

| Origin | Source | API |
|--------|--------|-----|
| `ExifJpeg` | EXIF IFD1 / APP1 JPEG | `try_exif_embedded_preview_*`, size probe |
| `PdfPageThumb` | PDF page `/Thumb` stream | `pdf_page_thumb_rgb` on size probe |

## Rules

1. **Never** insert embedded bytes into the tile pyramid.
2. Durable store uses `blob_lqip` with kind `kLqipKindEmbeddedJpeg` (3) only when
   no ThumbHash/Handsum is present for that blob/page.
3. `get_lqip` ignores kind 3; `get_embedded_preview` / `SizeReply.embedded` expose it.
4. Hosts must treat as underlay only and still schedule tiles.
5. Size probe may extract EXIF while already opening a JPEG for size — no extra
   full-image decode.

## SizeReply

```
size, lqip (ThumbHash/Handsum only), embedded (optional JPEG + origin)
```
