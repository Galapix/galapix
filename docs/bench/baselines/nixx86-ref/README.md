<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Baseline: nixx86-ref

Captured 2026-10-01T08:51Z via tools/capture_baselines.sh

- JPEG: photo_1920x1080_q90.jpg
- CBZ: book_pages.cbz
- repeats: 5

Compare:

```bash
python3 tools/compare_bench_json.py \
  --baseline docs/bench/baselines/nixx86-ref/gp-tile-jpeg.json \
  --current /tmp/gp-tile.json
```
