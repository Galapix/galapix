# Galapix / thumtoo performance measurement

Stop guessing which throttle hurts. Time phases.

## 1. Headless open path (no SDL / no GL)

Built as `extra/open_phase_bench` when `HAVE_THUMTOO` is on:

```bash
# Warm cache first (optional but useful for isolating Galapix vs generation)
thumtoo-prepare --cache ~/.cache/thumtoo album.zip   # or thumtoo-bench

open_phase_bench --cache ~/.cache/thumtoo album.zip other.zip
open_phase_bench --no-tiles --cache ~/.cache/thumtoo album.zip
```

Phases:

| Phase | What it measures |
|-------|------------------|
| `expand` | Archive TOC / PDF page expansion → per-member URIs |
| `size_probe` | Batch `request_size` + `drain` (cache hits should be near zero) |
| `providers` | `ThumtooTileProvider::create_from_size` for each URI |
| `overview_tiles` | `request_tile(max_scale, 0,0)` for each + drain (no GL upload) |

If `overview_tiles` is large with a warm cache, the cost is thumtoo get_tile /
decode / worker scheduling — not Galapix GL caps.
If `size_probe` dominates on a warm cache, locator/meta lookup is the issue.

## 2. Real viewer open (includes layout + first frames)

```bash
GALAPIX_OPEN_TIMING=1 galapix album.zip
```

Prints `[open-timing] <phase>: Δs (total …)` for:

- `expand`
- `size_probe`
- `add_images` (Image / TileProvider construction)
- `viewer_session` (full interactive session until quit)

First-paint latency after `launch_viewer` is **not** split yet (needs frame
hooks); use `open_phase_bench` for pre-viewer work and the HUD tile counters
for post-open pending requests/uploads.

## 3. thumtoo-only (generation vs cache hit)

```bash
thumtoo-bench --cache /tmp/t --tile-cell --ladder 256 album.zip
```

See `thumtoo` tool help for pyramid / job counts.

## 4. Interpreting multi-second “initial load”

1. Run `open_phase_bench` on the same zips (warm cache).
2. If total ≪ multi-second UI delay → cost is **viewer** (layout, first
   frames, GL upload, overview jobs), not open pipeline.
3. If `overview_tiles` is multi-second on warm cache → thumtoo/worker path.
4. If `expand` is multi-second → archive TOC I/O.

## Interpreting a real run (example)

```
expand:      0.0002 s
size_probe:  0.018 s   (1064 urls, 0 probed → warm cache)
add_images:  0.13 s
viewer_session_until_quit: 6.6 s   ← entire session, NOT first paint
```

Pre-viewer work here is **~0.15 s**. The multi-second “initial load” is
**inside the viewer**. With `GALAPIX_OPEN_TIMING=1` the viewer also logs:

* `viewer first draw`
* `frame N t=… req=… upload_q=… cache=…` (early frames)
* `first_cache_entry` — first successful tile surface in any cache
* `pending_idle` — request+upload queues empty after having been busy
