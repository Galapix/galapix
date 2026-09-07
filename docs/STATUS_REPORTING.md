<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Status reporting

Operators need to know **what the app is doing**: how many tiles are queued,
whether thumtoo is still cutting, size probes still running, etc. Today most of
that is only visible in the log.

## Current (console)

| Action | Key | Output |
|--------|-----|--------|
| Print visible image URLs | `Space` | stdout |
| Print tile / view load stats | `l` | stdout (`print_state`) |
| Print per-image info | `0` | stdout |

`Viewer::print_state` (key **`l`**) writes to **stdout** (not `log_info` — that is silent without `--verbose`) and reports:

* view offset and scale
* aggregate **pending tile requests** (SURFACE_REQUESTED)
* **pending GL uploads** (decoded tiles in `ImageTileCache::m_tile_queue`)
* **cache entry** counts across all workspace images
* **overview** counts (idle / loading / ready / failed) — stdio files only
* **thumtoo callback queue** depth when `HAVE_THUMTOO` (usually 0 with the
  default inline executor)

`Image::print_info` also prints per-image tile cache counters.

This is enough for debugging load policy without on-screen text.

## On-screen overlay (not implemented)

Needs **text rendering** in the SDL/OpenGL path (wstdisplay). Options, ordered
for this codebase’s preference for small dependency surface:

| Library | Pros | Cons | Fit |
|---------|------|------|-----|
| **stb_truetype** (+ optional **stb_easy_font**) | Single header, no pkg, atlas easy | You own layout/atlas/kerning | **Best first step** for debug HUD |
| **FreeType** | Full TTF, nixpkgs-friendly | Extra dep; still need atlas + GL upload | Good if quality/i18n matters |
| **SDL_ttf** | Simple API | Surface-oriented; less natural for pure GL path | OK for SDL-only experiments |
| **Dear ImGui** | Full debug UI, queues, plots | Heavier; another interaction model | Overkill unless we want a real debug panel |
| **Pango/Cairo** | High quality | Tied to GTK stack; wrong for default SDL package | Only if GTK frontend drives UI |

Recommendation: start with **stb_truetype** (or a tiny fixed bitmap font) drawn
into a GL texture each frame or on change. Keep the same counters already
exposed for `print_state`. Do **not** pull ImGui until a richer debug UI is an
explicit goal.

Possible HUD lines:

```text
tiles req=12 up=3 cache=480  thumtoo_q=0  scale=0.42
```

## thumtoo-side stats (future)

`thumtoo::Client` job backlog (size probes, tile cuts, archive opens) is not
yet queried from Galapix. When Client gains a cheap stats snapshot, surface it
next to the tile counters above. Archive coalesce and single-cell cut remain
**upstream thumtoo** work (see [TILE_LOADING.md](TILE_LOADING.md)).

## Related

* [TILE_LOADING.md](TILE_LOADING.md) — load policy and feature gaps
* [THUMTOO.md](THUMTOO.md) — backend flags and threading
* TODO.md — checklist items for overlay / queue visibility
