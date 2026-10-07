<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# RAR/CBR fixtures for unarr vs libarchive

`thumtoo-gp-archive --backend unarr` needs **RAR4** (not RAR5). unarr does not
support RAR5; `auto` falls back to libarchive for RAR5.

## Produce a small RAR4 (manual)

With proprietary `rar` (RARLabs):

```bash
# From a few benchtoo JPEGs
mkdir -p /tmp/rar-in && cp /path/to/benchtoo/synthetic/jpeg/comic_600x900_q90.jpg /tmp/rar-in/
rar a -m3 -ma4 /tmp/comic_sample.rar /tmp/rar-in/*    # -ma4 = RAR4
thumtoo-gp-archive --backend unarr --json /tmp/comic_sample.rar
thumtoo-gp-archive --backend libarchive --json /tmp/comic_sample.rar
```

Solid RAR4 (`rar a -s …`) is the interesting case: libarchive often cannot
extract solid payloads while unarr can.

## Corpus policy

Do **not** commit binary RAR blobs into thumtoo or benchtoo source without a
clear license and size budget. Prefer:

1. Document the generator command (this file)
2. Optional third-party flake input for licensed samples
3. CI secret/cache of a pinned fixture tarball

benchtoo may later grow `generators/gen_rar.py` that shells out to `rar` when
present and no-ops otherwise (same pattern as pdf2djvu).
