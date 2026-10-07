#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerate the DjVu test fixtures (committed; tests do not need the tools).
# Needs djvulibre's encoders (c44, cjb2, djvumake, djvm) and python3 — all in
# `nix develop`.
#
#   tests/fixtures/make_djvu_fixtures.sh
#
# pages.djvu (bundled, 3 pages, Letter):
#   1  bitonal  2550x3300 @ 300 dpi   JB2 text page (Sjbz)
#   2  photo    1275x1650 @ 150 dpi   IW44 colour page (BG44)
#   3  compound 2550x3300 @ 300 dpi   JB2 mask + 100 dpi IW44 background
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

python3 - "$work" <<'EOF'
import sys
out = sys.argv[1]

def pbm(path, w, h):
    # Text-like rows of glyph blocks: deterministic, compresses well.
    rows = []
    row_bytes = (w + 7) // 8
    for y in range(h):
        line = bytearray(row_bytes)
        band = (y % 50)
        if 150 < y < h - 150 and 8 < band < 34:
            for x in range(150, w - 150):
                if ((x // 18) % 3 != 2) and ((x * 7 + y * 3) % 11 < 7):
                    line[x // 8] |= 0x80 >> (x % 8)
        rows.append(bytes(line))
    with open(path, "wb") as f:
        f.write(b"P4\n%d %d\n" % (w, h))
        f.write(b"".join(rows))

def ppm(path, w, h):
    data = bytearray()
    for y in range(h):
        for x in range(w):
            data += bytes(((x * 255) // w, (y * 255) // h, ((x + y) * 3) % 256))
    with open(path, "wb") as f:
        f.write(b"P6\n%d %d\n255\n" % (w, h))
        f.write(bytes(data))

pbm(f"{out}/text.pbm", 2550, 3300)
ppm(f"{out}/photo.ppm", 1275, 1650)
ppm(f"{out}/bg.ppm", 850, 1100)
EOF

cjb2 -dpi 300 "$work/text.pbm" "$work/bitonal.djvu"
c44 -dpi 150 "$work/photo.ppm" "$work/photo.djvu"
c44 -dpi 100 "$work/bg.ppm" "$work/bg.djvu"
djvuextract "$work/bitonal.djvu" Sjbz="$work/mask.jb2"
djvuextract "$work/bg.djvu" BG44="$work/bg.iw44"
djvumake "$work/compound.djvu" INFO=2550,3300,300 Sjbz="$work/mask.jb2" \
  FGbz=#000000 BG44="$work/bg.iw44"
djvm -c "$here/pages.djvu" "$work/bitonal.djvu" "$work/photo.djvu" "$work/compound.djvu"
ls -l "$here/pages.djvu"
