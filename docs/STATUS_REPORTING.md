<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Status reporting

Operators need to know **what the app is doing**: how many tiles are queued,
whether thumtoo is still cutting, size probes still running, etc.

## Console

| Action | Key | Output |
|--------|-----|--------|
| Print visible image URLs | `Space` | stdout |
| Print tile / view load stats | `l` | stdout (`print_state`) |
| Print per-image info | `0` | stdout |
| Toggle ImGui chrome (tools + status) | `Tab` / `F1` | on-screen |

`Viewer::print_state` (key **`l`**) writes to **stdout** (not `log_info` — that is silent without `--verbose`) and reports:

* view offset and scale
* aggregate **pending tile requests** (SURFACE_REQUESTED)
* **pending GL uploads** (decoded tiles in `ImageTileCache::m_tile_queue`)
* **cache entry** counts across all workspace images
* **overview** counts (idle / loading / ready / failed) — stdio files only
* **thumtoo callback queue** depth when `HAVE_THUMTOO` (usually 0 with the
  default inline executor)

`Image::print_info` also prints per-image tile cache counters and overview state.

## On-screen overlay (Dear ImGui)

**Dear ImGui** is the SDL/OpenGL chrome overlay (vendored under `external/imgui`,
v1.91.6). Toggle with **Tab** or **F1**.

* Pure OpenGL draw (no native widgets) — temporary shell while the long-term
  UI stays spatial on the canvas.
* Same counters as key **`l`**: view scale/offset, tile backlog, overview state.
* Input: `WantCaptureMouse` / `WantCaptureKeyboard` gate viewer tools so
  pan/zoom still work outside ImGui windows.
* Application state stays in `Viewer` / `Workspace`; ImGui only displays and
  may later send commands (do not bury logic in widgets).

stb_truetype is available inside ImGui (`imstb_truetype.h`) if custom text is
needed later; a separate stb-only HUD is not required.

## thumtoo-side stats (future)

`thumtoo::Client` job backlog (size probes, tile cuts, archive opens) is not
yet queried from Galapix. When Client gains a cheap stats snapshot, surface it
next to the tile counters above. Archive coalesce and single-cell cut remain
**upstream thumtoo** work (see [TILE_LOADING.md](TILE_LOADING.md)).

## Related

* [TILE_LOADING.md](TILE_LOADING.md) — load policy and feature gaps
* [THUMTOO.md](THUMTOO.md) — backend flags and threading
* TODO.md — checklist items for overlay / queue visibility
* `external/imgui/README.galapix.md` — vendor note
