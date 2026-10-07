<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Tile edge overlap — removed

There is **no** `kTileOverlap`. Cells are exclusive **≤256×256**. Grid step is
`kTileSize`.

The 257 / paint-expand / encode-strip / overscan experiments are gone. Do not
reintroduce them.

PDF page tiles: each cell is a region render of the page's shared display
list with the cell as the device rect; images are decoded whole once, which
is what keeps neighbouring cells seam-free (see TILES.md "PDF rendering"). Layout pixels are a single `lround(page_pt * dpi/72)` from the
continuous page bound — not integer 72dpi points then scaled again.
