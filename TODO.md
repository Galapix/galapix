## PDF backend-aware min_scale — **galapix-165**

- [x] `pdf_page_allows_live_tiles(..., parsed->backend)`
- [x] Log backend name when gating image-heavy pages
- [x] Bundle galapix-165

---

## PDF image-heavy min_scale — **galapix-164**

- [x] `create_from_size`: image-heavy PDF pages use min_scale=0 (no live deep tiles)
- [x] Bundle galapix-164

---

## Tile request queue dump — **galapix-163**

- [x] `ImageTileCache::dump_request_queue` — in-flight cells by age/scale
- [x] Wired through Image / Workspace; `print_state` (`l`) and frame-timing stuck dump
- [x] Status panel points at stdout dump
- [x] Bundle galapix-163

---

## LQIP timing (not display hacks) — **galapix-162**

- [x] Revert warm/cold LQIP display special-case
- [x] Document: generation after first durable tile is the whole mechanism
- [x] Bundle galapix-162

---

## GUI: no source I/O on main thread (2026-09-09) — **galapix-160**

Sleeping USB can take ~30s to spin up. open_paths expansion and
pdf_page_count/archive TOC must not run on the GUI thread.

- [x] `open_paths` expands on a detached worker; `process_pending_opens` on GUI
- [x] No `ThumtooTileProvider::create`+drain from open_paths
- [x] ImageOverview `get_lqip` miss backoff 250ms
- [x] SizeProbe attach for pending opens without size
- [x] Bundle galapix-160

---

## GUI: get_lqip only in ImageOverview (2026-09-08) — **galapix-082**

After skipping LQIP in size probe, ensure_lqip on every prepare/draw frame
rasterized every visible DjVu page on the UI thread (100% CPU, no updates).

- [x] get_lqip only in ImageOverview
- [x] Bundle galapix-082

---

## Accept DjVu in directory/file scan (2026-09-08) — tip **galapix-157** / bundle **galapix-075**

`Filesystem::generate_image_file_list` only allowed PDF + archives past the
surface_factory filter. Opening a `.djvu` (or a dir of them) yielded
"0 files found" / "No URLs given" before ViewerCommand expansion.

- [x] Code: `is_likely_djvu_path` in scan filter
- [x] Bundle galapix-075

---

## Include thumtoo/djvu.hpp for page expand (2026-09-08) — tip **galapix-155** / bundle **galapix-073**

viewer_command / viewer called `thumtoo::is_likely_djvu_path` without including
`<thumtoo/djvu.hpp>` (only pdf.hpp was pulled in).

- [x] Code
- [x] Bundle galapix-073

---

## DjVu open path (2026-09-08) — tip **galapix-154** / bundle **galapix-072**

thumtoo-064 adds DjVu page tiles. Galapix only needs:

- Expand `.djvu`/`.djv` to `file://…//page:N` (viewer + ViewerCommand)
- Desktop MIME: `image/vnd.djvu`, `image/vnd.djvu+multipage`
- URI bridge uses `djvu_page_uri` when path is DjVu

Requires thumtoo with `THUMTOO_HAVE_DJVU` (DjVuLibre).

- [x] Code
- [x] Bundle galapix-072

---

## Fix xdgcpp link + decouple SizeProbe from ImGui (2026-09-08) — tip **galapix-153** / bundle **galapix-071**

Build failures after galapix-070:

1. `xdg.h: No such file or directory` — CMake linked bare `xdgcpp`; the
   exported target is `xdgcpp::xdgcpp` (INTERFACE include dirs only on that).
2. `undefined reference to ImguiOverlay::notify` when linking benchmarks —
   `SizeProbeSession` (libgalapix) pulled in the app-layer ImGui overlay.

### Fix
- Link `xdgcpp::xdgcpp`
- Add `util/status_notify` hook in libgalapix; SizeProbe calls `status_notify()`
- `ImguiOverlay::init` registers the handler; shutdown clears it
- App code may still call `ImguiOverlay::notify` directly

- [x] Code
- [x] Bundle galapix-071

---

## ImGui.ini under XDG config (2026-09-08) — tip **galapix-152** / bundle **galapix-070**

Stop dumping `imgui.ini` into the process cwd. Use the XDG Base Directory
spec via [xdgcpp](https://github.com/Grumbel/xdgcpp):

  `$XDG_CONFIG_HOME/galapix/imgui.ini`  (fallback `~/.config/galapix/imgui.ini`)

- Add flake input `xdgcpp` and link it into `libgalapix_app`
- Set `ImGuiIO::IniFilename` in `ImguiOverlay::init`; create the config dir
- Do **not** touch legacy `~/.galapix` (galapix-0.2.x leftover; no migration)
- After pull: `nix flake lock --update-input xdgcpp` (lock not refreshed in this
  environment — no nix binary)

- [x] Code
- [x] Bundle galapix-070

---

## Revert ViewerState double + camera-relative (2026-09-08) — tip **galapix-151** / bundle **galapix-069**

Get rid of the two recent commits that switched pan/zoom to double and added
camera-relative world→draw transforms. Back to plain float `ViewerState`.

Higher precision (if needed later) will be done differently, e.g. making
wstdisplay path double — not by splitting float/double at the matrix boundary
or subtracting a render origin.

- Restored pre-37a34d1 sources for ViewerState, ImageRenderer, tools, mandelbrot, etc.
- Kept current flake.lock
- Removed Vector2d/Rectd aliases and to_draw / set_render_origin helpers

- [x] Code
- [x] Bundle galapix-069

---

## Camera-relative float matrices (2026-09-08) — tip **galapix-150** / bundle **galapix-068**

Synced to upstream `f6f531b2`. Float modelview still breaks deep zoom if world
positions are large — GPU uniforms are float32; wstdisplay `glm::mat4` stays.

### Approach
- Keep `ViewerState` in **double**
- Modelview translation = `offset + scale * view_origin` (double → float)
- Draw geometry as `float(world - view_origin)` via `ImageRenderer::set_render_origin`

Not a full double path through OpenGL (would still quantize at the GPU).

- [x] Code
- [x] Bundle galapix-068

---

## ViewerState double precision (2026-09-08) — tip **galapix-149** / bundle **galapix-067**

View pan/zoom used float; deep `builtin://mandelbrot` zoom falls apart.

### Change
- `ViewerState` offset/scale/angle are **double** (`Vector2d`, `Rectd`)
- OpenGL still gets float at the matrix boundary (`get_*_f` / casts)
- Tools use `screen2world_f` for selection geometry
- Mandelbrot tile coords in double; more iterations at fine scales
- Workspace item layout remains float (not full-world double yet)

- [x] Code
- [x] Bundle galapix-067

---

## Revert LIFO-starve timeout; rely on thumtoo FIFO (2026-09-08) — tip **galapix-148** / bundle **galapix-066**

Drop age-based REQUESTED retry. Completion is deterministic via thumtoo-062
FIFO interactive tile queue + same-cell coalesce.

- [x] Code
- [x] Bundle galapix-066

---

## Retry LIFO-starved tile REQUESTED (2026-09-08) — tip **galapix-147** / bundle **galapix-065**

Archive/grid cells could sit REQUESTED 20s+ forever: thumtoo interactive
queue is LIFO, so continuous new work starves older jobs. Treat live
REQUESTED older than 20s as failed and re-issue (up to 3 attempts).

- [x] Code
- [x] Bundle galapix-065

---

## SizeProbeSession access + Workspace init (2026-09-08) — tip **galapix-146** / bundle **galapix-064**

- Make `Image::set_tile_provider` public for async size probe attach
- Initialize `Workspace::m_size_probe` in the ctor for `-Weffc++`

- [x] Code
- [x] Bundle galapix-064

---

## Fix SizeProbeSession compile (2026-09-08) — tip **galapix-145** / bundle **galapix-063**

- Match `start_drain(client, self)` / `tick()` to the header
- Explicit member init list for `-Weffc++`

- [x] Code
- [x] Bundle galapix-063

---

## README spelling: builtin:// (2026-09-08) — tip **galapix-144** / bundle **galapix-062**

- Fix `buildin://mandelbrot` → `builtin://mandelbrot`

- [x] Code
- [x] Bundle galapix-062

---

## Async thumtoo size probe + UI status (2026-09-08) — tip **galapix-143** / bundle **galapix-061**

Blocking `Probing image sizes (thumtoo)...` + `drain()` before launch made large
opens feel hung.

- Queue `request_size`, open viewer immediately with placeholder Images
- Background thread runs `Client::drain()`
- Main-thread `SizeProbeSession::tick` attaches providers as `get_size` fills in
- Console progress + ImGui toasts; relayout when complete

- [x] Code
- [x] Bundle galapix-061

---

## Update README (2026-09-08) — tip **galapix-142** / bundle **galapix-060**

- Remove Travis/Coverity badges and leftover "zoomview" title fragment
- Document Nix flake + CMake (no waf), thumtoo, current deps and CLI
- Default geometry note 1024x768

- [x] Code
- [x] Bundle galapix-060

---

## Default window size 1024x768 (2026-09-08) — tip **galapix-141** / bundle **galapix-059**

- Pulled `origin/master` (includes flake.lock update and reclaim removal)
- Default `Options::geometry` 800x600 → **1024x768** (`-g` / `--geometry` still overrides)
- Removed older handoff bundles 051–058 from artifacts (keep tip 059)

- [x] Code
- [x] Bundle galapix-059

---

## Remove timed tile REQUESTED reclaim (2026-09-08) — tip **galapix-140** / bundle **galapix-058**

The 4s `reclaim_stuck_requests` path was a workaround for lost callbacks; it
also kills legitimate in-flight work when many images/archives are loading.

- Remove timed reclaim; rely on cancel_jobs + provider always completing handles
- Keep dump_stuck for GALAPIX_OPEN_TIMING diagnostics only

- [x] Code
- [x] Bundle galapix-058

---

## Rewrite overlap solver (2026-09-08) — tip **galapix-139** / bundle **galapix-057**

Old `Workspace::solve_overlaps` was O(n²) full pairwise + unbounded loop with
`NumOverlappings` spam; unusable on large collections.

New `solve_image_overlaps` (layouter/overlap_solver):
- Spatial hash neighbor queries
- Iterative minimum-translation axis separation (max 64 passes)
- 16px gap; writes positions back once per run
- No per-iteration stdout

- [x] Code
- [x] Bundle galapix-057

---

## Stop overview levels budget thrash (2026-09-08) — tip **galapix-138** / bundle **galapix-056**

Stable gallery zoom still showed `new_req=128 req=0` after the tile retry
fix. Root cause: `ImageOverview::ensure_levels` on LQIP failure set
`m_levels_requested = false`, so every visible image re-spent 1 unit of the
global request budget each frame (not counted in `req`).

- Keep `m_levels_requested` after failure; skip if already requested
- Raise LQIP-only postage threshold 64 → 128 px

- [x] Code
- [x] Bundle galapix-056

---

## Stop failed-tile request thrash (2026-09-08) — tip **galapix-137** / bundle **galapix-055**

Frame timing at gallery zoom-out showed `new_req=128 budget_left=0` with
`req=0 upl=0` every frame — budget burned with no live work.

Cause: after `kMaxTileAttempts` failures, `issue_requests` erased the dead
entry and immediately re-queued the same cell next frame (infinite fail loop).

- Keep exhausted dead entries until `cancel_jobs` (scale/rect change)
- Same for `queue_tile_request`
- Raise adjacent scale hold 50ms → 150ms to reduce intermediate grid re-issue

- [x] Code
- [x] Bundle galapix-055

---

## Frame timing diagnostics (2026-09-08) — tip **galapix-136** / bundle **galapix-054**

`GALAPIX_FRAME_TIMING=1` per-frame phase breakdown (pump / prepare / draw ms)
plus upload and request counters. Helps diagnose zoom-out frame drops.

- [x] Code + STATUS_REPORTING.md
- [x] Bundle galapix-054

---

## Fix WERROR: third-party SYSTEM + remaining app warnings (2026-09-08) — tip **galapix-135** / bundle **galapix-053**

Remaining failures after 051/052 under WARNINGS=ON WERROR=ON:

- ImGui headers diagnosed when included from `imgui_overlay.cpp` → mark
  imgui include dirs **SYSTEM**; keep `-w` on imgui .cpp sources
- thumtoo `Executor` default ctor (-Weffc++) via non-system include → mark
  thumtoo interface includes **SYSTEM**
- `ThumtooCallbackQueue` / `ImguiOverlay` member-init list (-Weffc++)
- `AppViewer` lambda param shadows ctor param (-Wshadow)
- `ImGui::Text` float→double (-Wdouble-promotion)

- [x] Code
- [x] Bundle galapix-053

---

## Fix WERROR: ThreadMessageQueue2 size + ViewerCommand init (2026-09-08) — tip **galapix-134** / bundle **galapix-052**

- `ThreadMessageQueue2::size()`: cast `queue::size_type` → `int` (-Wconversion)
- `ViewerCommand`: init `m_thumtoo` in member-init list under HAVE_THUMTOO (-Weffc++)

- [x] Code
- [x] Bundle galapix-052

---

## Fix WERROR build: Image override + effc++ (2026-09-08) — tip **galapix-133** / bundle **galapix-051**

With WARNINGS=ON WERROR=ON, Image and ImageOverview failed:

- `Image::request_overview_levels` missing `override`
- `Image` has raw pointer `m_job_manager` → need deleted copy/assign (-Weffc++)
- `ImageOverview` default ctor left `m_queue` / `m_surface` uninitialized in
  member-init list (-Weffc++)

- [x] Code
- [x] Bundle galapix-051

---

## galapix-configure: WARNINGS=ON WERROR=ON (2026-09-08) — tip **galapix-132** / bundle **galapix-050**

Dev `galapix-configure` passes `-DWARNINGS=ON -DWERROR=ON` so warnings fail the build.

- [x] flake.nix
- [x] Bundle galapix-050

---

## Fix pending_tile_requests on WorkspaceItem (2026-09-08) — tip **galapix-131** / bundle **galapix-048**

Compile error: `WorkspaceItem` had no `pending_tile_requests`. Add virtual
default 0; `Image` overrides.

- [x] Code
- [x] Bundle galapix-048

---

## Reclaim + dump stuck REQUESTED tiles (2026-09-08) — tip **galapix-130** / bundle **galapix-047**

Open-timing showed `req=15` for 40s with `upload_q=0` — live REQUESTED
handles that never finished/failed (lost backend callback).

### Changes
- `issued_at` on each REQUESTED entry
- `reclaim_stuck_requests()` after 4s: set_failed + erase + log
- `dump_stuck_requests()` printed under GALAPIX_OPEN_TIMING when req>0
- Off-screen images with pending requests still run process_queue/reclaim

Not a full data-driven rewrite; isolates lost jobs so they stop blocking
the counter and can be re-issued on the next mark.

- [x] Code
- [x] Bundle galapix-047

---

## Only request view-scale tiles; cancel other REQUESTED (2026-09-08) — tip **galapix-129** / bundle **galapix-046**

### Problem
`mark_tile_needed` queued the whole ancestor chain. `cancel_jobs` kept
coarser REQUESTED jobs as "stand-ins". Result: pending_requests stuck high,
tile-debug mixed resolutions that never settled to the view scale.

### Fix
- Mark **only** the exact view-scale cell
- Cancel **all** REQUESTED jobs at other scales (or outside the visible rect)
- Stand-ins still come from already-SUCCEEDED coarser tiles via
  `find_smaller_tile` — no extra requests

- [x] Code
- [x] Bundle galapix-046

---

## Fix Viewer JobManager shutdown assert (2026-09-08) — tip **galapix-128** / bundle **galapix-045**

`Thread::~Thread` asserts `m_state == kJoined`. Viewer only called
`stop_thread()` (sets quit, no join), so exit aborted after adding a
JobManager to Viewer for drag-and-drop.

Match ViewerCommand: `abort_thread()` + `join_thread()`.

- [x] Code
- [x] Bundle galapix-045

---

## Verify + fix Viewer JobManager threads (2026-09-08) — tip **galapix-127** / bundle **galapix-044**

Verification of galapix-043 / thumtoo-060:

- Index-based batch delivery: OK
- Reply-before-durable + deferred store: OK
- Probe once / skip_probe: OK
- pending excludes finished/failed: OK
- Drag-drop open_paths + SDL_DROPFILE: OK
- No-args start: OK

**Bug found:** `Viewer` constructed `JobManager(0)` which `assert(num_threads > 0)`.
Drop-opened images would crash or have no overview workers. Fixed to 2 threads.

- [x] JobManager threads fix
- [x] Bundle galapix-044

---

## Index-based batch tile delivery (2026-09-08) — tip **galapix-126** / bundle **galapix-043**

Requires thumtoo-060. ThumtooTileProvider::request_tiles delivers by index
so every JobHandle is finished/failed; tiles can reach the upload queue.

- [x] Code
- [x] Bundle galapix-043

---

## Drag-and-drop open images and archives (2026-09-08) — tip **galapix-125** / bundle **galapix-042**

SDL_DROPFILE → `Viewer::open_paths`: files, directories, archives (image members),
PDFs (pages). Same expansion as CLI. Enables `SDL_EventState(SDL_DROPFILE)`.

- [x] Code
- [x] Bundle galapix-042

---

## No-args start + pending_requests excludes finished (2026-09-08) — tip **galapix-124** / bundle **galapix-041**

- `requires_command_line_args()` → false: `galapix` with no args opens the viewer
- `pending_request_count` ignores finished handles (decoded, waiting GL upload)

Fresh tile generate left many REQUESTED+finished cells counted as pending until
upload drained; cache hits uploaded so fast it was barely visible.

- [x] Code
- [x] Bundle galapix-041

---

## Pending-request accounting + dead cell cleanup (2026-09-08) — tip **galapix-123** / bundle **galapix-040**

- `pending_request_count` ignores failed/aborted handles (was stuck ~100)
- Exhausted retry cells are erased so they can be re-issued
- Pair with thumtoo-059 batch probe/exception fixes

- [x] Code
- [x] Bundle galapix-040

---

## Fix TileRequest / JobHandle default-ctor (2026-09-08) — tip **galapix-122** / bundle **galapix-039**

`JobHandle()` is private; `TileRequest req;` failed to compile. Explicit
constructor + `emplace_back` in `issue_requests`.

- [x] Code
- [x] Bundle galapix-039

---

## Build fix for tile batch API (2026-09-08) — tip **galapix-121** / bundle **galapix-038**

- JobHandle default member on TileRequest → `JobHandle::create()`
- Declare `ThumtooTileProvider::request_tiles`

- [x] Code
- [x] Bundle galapix-038

---

## Batch visible tiles in one thumtoo job (2026-09-08) — tip **galapix-120** / bundle **galapix-037**

### Problem
Even after finer-first issue order, a deep-zoomed image still took ~1s for
all cells to appear. Each cell was a separate Client job; workers contended
on the per-image shrink ladder and paid queue overhead N times.

### Fix
- thumtoo-057: `Client::request_tiles` — one job, many cells
- `TileProvider::request_tiles` + `ThumtooTileProvider` override
- `ImageTileCache::issue_requests` builds a batch and sends it in one call

Requires thumtoo-057.

### Status
- [x] Code
- [x] Bundle galapix-037

---

## Finer-first tile issue + higher start budget (2026-09-08) — tip **galapix-119** / bundle **galapix-036**

### Problem
Zoomed into one image, the first tile appears reasonably quickly but the rest
trickle in over seconds. Coarser-first `issue_requests` + marking full ancestor
chains spent the per-frame start budget on overview/parent jobs; exact cells
started late. Budget 48/frame made a large visible grid take several frames to
even *enqueue*.

Throttling was aimed at gallery stampede; it hurts the single-image deep-zoom
case. Provider work was already off-GUI; order/budget were wrong.

### Fix
- Issue **finer scales first** (exact view before parents)
- Mark ancestors only when not already SUCCEEDED (no budget fight with hits)
- Raise per-frame start budget **48 → 128**

### Status
- [x] Code
- [x] Bundle galapix-036

---

## Tile request LIFO catch-up + zoom hold (2026-09-08) — tip **galapix-118** / bundle **galapix-035**

### Problem
With tile requests on (not `U`), pan/zoom drops frames; after stopping, the
view takes long to fill because thumtoo's job queue was **FIFO** — intermediate
cells from the gesture ran before the final view. `cancel_jobs` only marks
Galapix handles aborted; work already queued in thumtoo still runs to completion.

Provider I/O/decode was already off the GUI thread; the pain is backlog order
and enqueueing intermediate zoom grids.

### Fix
- **thumtoo-056**: interactive `request_tile` enqueues **LIFO** (`push_front`);
  supersede pending same-cell jobs; bulk pyramid stays FIFO
- **Galapix**: while `stable_request_scale` is holding, cancel only (no mark/issue)
- **Galapix**: `issue_requests` prefers cells nearer the viewport centre

### Status
- [x] Code
- [x] Bundle galapix-035
- Requires thumtoo-056

---

## Prune arxpcpp/exspcpp from flake.lock (2026-09-08) — tip **galapix-117** / bundle **galapix-034**

flake.nix no longer inputs arxpcpp/exspcpp, but flake.lock still locked those
nodes and routed tinycmmc follows through `exspcpp`. Removed the nodes and
rewired tinycmmc to root `nixpkgs` / `flake-utils`.

### Status
- [x] flake.lock prune
- [x] Bundle galapix-034

---

## Verify arxpcpp removal + dead uitest cleanup (2026-09-08) — tip **galapix-116** / bundle **galapix-033**

Follow-up to galapix-115:

- Confirmed no `arxp` in src/CMake/flake.nix
- Deleted uitests for removed resource/ArchiveThread stack (already unbuildable)
- `curl_test` writes via ofstream instead of `arxp::write_file`
- CACHE4_VS_THUMTOO notes updated

Remaining historical mentions in older TODO sections / NEWS are archival only.

### Status
- [x] Verification + uitest cleanup
- [x] Bundle galapix-033

---

## Remove arxpcpp / exspcpp — archives via thumtoo (2026-09-08) — tip **galapix-115** / bundle **galapix-032**

### Goal
All archive TOC / member access goes through thumtoo (libarchive). Drop
arxpcpp and transitive exspcpp from Galapix.

### Galapix changes
- [x] `Filesystem::generate_image_file_list`: drop arxp expand; archives already
      added via `is_likely_archive_path` and expanded in `ViewerCommand`
- [x] `URL::get_data`: plain file via `Blob::from_file`; archive plugins via
      `thumtoo::extract_archive_member` (HAVE_THUMTOO)
- [x] Remove `App::archive` / `ArchiveManager` wiring
- [x] Delete idle `ArchiveThread`
- [x] CMake + flake: drop arxp / exspcpp
- [x] Update DEPENDENCIES.md

### Preserve later in thumtoo (not Galapix)
- unrar for RAR subformats libarchive cannot open
- optional external unrar/unzip for single-member extract (avoids full TOC walk)

### Status
- [x] Code
- [x] Bundle galapix-032

---

## Overview underlay kind (2026-09-08) — tip **galapix-114** / **galapix-031**

Replace `m_lqip_only` bool with `Underlay { None, Lqip, Levels }` so load
`State` (Idle/Loading/Ready/Failed) is orthogonal to what is drawn.
`is_lqip_only()` → `m_underlay == Lqip` (skip_grid still correct).

- [x] Code
- [x] Bundle galapix-031

---

## SQLite connection mutex (2026-09-08) — **thumtoo-051**

- [x] Database + BlobStore recursive_mutex on public methods

---

## Audit continuation (2026-09-08) — tip **galapix-113**

### Checked
- [x] blob_store.cpp (WAL, schema, multi-worker single-connection model)
- [x] archive.cpp / pdf.cpp structure
- [x] Document findings in AUDIT.md

### Still open
- [ ] Explicit db mutex or per-worker SQLite connections (if TSan noise / contention)
- [ ] Overview state machine cleanup
- [ ] PDF LQIP
- [ ] Blob eviction policy

---

## Archive warm-path + thread atomics (2026-09-08) — tip **galapix-112**

### Done
- [x] thumtoo-049: EnsureTiles batch skip extract on get_tile hit
- [x] thumtoo-050: EnsurePixels + ProbeSize warm batch skip extract
- [x] galapix-027: atomic shutdown flags (ThreadPool, AsyncMessenger,
      DownloadManager, SpaceNavigator)

### Next (optional)
- [ ] Overview state machine cleanup (Idle/Lqip/Levels/Failed)
- [ ] blob_store deep dive
- [ ] PDF LQIP

---

## Handsum LQIP (c3q4) for sharper placeholders (2026-09-08) — tip **galapix-111**

Prefer Handsum (147 B fixed) over ThumbHash for content.lqip. Decode via
`lqip_decode_rgba` (magic-detect). ensure_lqip upgrades old ThumbHash rows.

Requires thumtoo with handsum.cpp (local path / next thumtoo bundle).

Clear or open images so ensure_lqip rewrites LQIP blobs.

### Status
- [x] thumtoo encode/decode + kind
- [x] Galapix decode path
- [x] Bundles galapix-026+ / thumtoo-046…050

---

## Grid tiles starved by levels budget (2026-09-08) — tip **galapix-110** / bundle **galapix-025**

### Symptom
LQIP appears; real tiles only after zoom in/out. Otherwise LQIP sticks.

### Cause
`ensure_requested` spent the global per-frame request budget on
`request_pixels` (levels) during prepare, before `issue_tile_requests`.
Grid jobs saw budget 0 every frame until zoom re-ordered work.

### Fix
- `ensure_requested`: LQIP (+ non-thumtoo overview) only — no tile budget
- New `ensure_levels` / `Image::request_overview_levels`
- Workspace pass 3: levels **after** grid `issue_tile_requests` (remaining budget)

### Status
- [x] Code
- [x] Bundle galapix-025

---

## Grid tiles blocked after LQIP (2026-09-08) — tip **galapix-109** / bundle **galapix-024**

### Symptom
LQIP underlays appear, but never upgrade to real image tiles.

### Causes
1. `prepare` refused to mark grid tiles while overview was `Loading` — levels
   upgrade after LQIP set Loading and blocked the grid forever if levels failed
   or stalled.
2. Levels failure with LQIP kept `Loading` (did not restore `Ready`).
3. Gallery `skip_grid` treated any Ready underlay (including LQIP-only) as
   sufficient, so the one-cell grid never ran.

### Fix
- Mark grid when any underlay surface exists (even if levels Loading).
- On levels failure with LQIP: restore Ready, clear m_levels_requested.
- skip_grid only for Ready non-LQIP (levels) underlay.

### Status
- [x] Code fix
- [x] Bundle galapix-024

---

## LQIP one-shot miss log (2026-09-08) — tip **galapix-108** / bundle **galapix-023**

When `ensure_lqip` returns empty, log once per overview instance (not every
frame). Helps distinguish "never had content_id/VIPS" from decode failures.

### Status
- [x] m_lqip_miss_logged + log_debug
- [x] Bundle galapix-023

---

## LQIP path debug logging (2026-09-08) — tip **galapix-107** / bundle **galapix-022**

Add log_debug when ThumbHash LQIP decodes, when decode/surface create fails,
and when LQIP GL upload succeeds. Misses before size probe are intentionally
not logged every frame (would flood gallery).

Run with `--debug` (or log level debug) to confirm:
- "LQIP decode WxH for …" then "LQIP GL upload ready WxH" → path works
- only decode/surface failures → encode/hash problem
- neither line → ensure_lqip empty (no content_id / VIPS miss) or not Thumtoo provider

### Status
- [x] Debug logs
- [x] Bundle galapix-022
- [ ] User runtime confirmation

---

## Thread-safety atomics + remove dead m_lqip_tried (2026-09-08) — tip **galapix-106** / bundle **galapix-021**

From source audit (AUDIT.md):
- JobHandle flags → `std::atomic<bool>` (safe cross-thread is_*)
- JobWorkerThread m_quit/m_abort → atomic
- Viewer::redraw coalesce → atomic CAS; ~Viewer clears current_
- Remove unused ImageOverview::m_lqip_tried

LQIP purple symptom: audit found no proven race; still needs runtime ensure_lqip hit/miss if it persists.

### Status
- [x] Atomics + dead code cleanup
- [x] Bundle galapix-021
- [ ] LQIP runtime confirmation (separate)

---

## Investigate LQIP not showing / only purple placeholders (2026-09-08)

Primary audit complete — see AUDIT.md. Thread-safety fixes applied above.

---

## Use ensure_lqip + same-frame overview process (2026-09-08) — tip **galapix-105** / bundle **galapix-020**

Warm caches had size but no ThumbHash; `get_lqip` stayed empty. Call
`Client::ensure_lqip` (thumtoo-045) to backfill. Run overview `process()` after
`prepare` so LQIP uploads the same frame it is decoded.

### Status
- [x] Bundle galapix-020
- Requires thumtoo-045

---

## LQIP retry + less opaque purple (2026-09-08) — tip **galapix-104** / bundle **galapix-019**

### Bugs
1. `m_lqip_tried` made `get_lqip` one-shot; size probe often finishes *after*
   the first `ensure_requested` → permanent miss → purple forever.
2. Cache-only mode returned before LQIP (which is cache-only).
3. Opaque purple painted while overview Loading (LQIP queued / levels in flight).

### Fix
- Retry `get_lqip` every frame until a surface exists
- Try LQIP even when tile requests disabled
- Opaque purple only when overview Idle/Failed with no surface; Loading leaves
  underlay visible; Ready underlay skips tile purple (debug tint only)

### Status
- [x] Bundle galapix-019

---

## Galapix paints ThumbHash LQIP under overview (2026-09-08) — tip **galapix-103** / bundle **galapix-016**

ImageOverview tries `Client::get_lqip` first (inline ThumbHash, no blob I/O).
At on-screen long edge ≤64, LQIP alone is enough (no levels stampede). Larger
views still `request_pixels` and replace the underlay when levels arrive.

### Status
- [x] Bundle galapix-016
- Requires thumtoo-040 (schema v2 + get_lqip)

---

## Overview max_edge matches display size (2026-09-08) — tip **galapix-102** / bundle **galapix-015**

Gallery of ~1000 images requested a **512** long-edge level for every thumbnail
while on-screen size was often ~50–100px → encode/decode/upload cost dominated
fill time (~3s).

`ensure_requested` now takes `target_long_edge` from displayed size, clamped to
**128..512**. Thumtoo `request_pixels` and libjpeg overview path both use it.

### Status
- [x] Bundle galapix-015

---

## Levels-first overview + purple while loading (2026-09-08) — tip **galapix-101** / bundle **galapix-012**

### Issues
1. Purple placeholders still missing in gallery: `skip_grid` was true for
   overview Loading/Idle, so `draw_tile` never ran.
2. Thumtoo overview used **coarsest grid tile** (`request_tile(max_scale)`), not
   the **levels** ladder (`request_pixels`). Levels never appeared.
3. Grid tiles were marked every frame while overview still loading → too early.

### Fix
* Gallery: `skip_grid` only when overview **Ready** with a surface
* Image-level purple fill while overview Idle/Loading and no surface
* Thumtoo overview: `Client::request_pixels(..., 512)` (JXL level)
* `prepare`: do not mark grid tiles until overview is Ready or Failed

### Status
- [x] Bundle galapix-012

---

## Purple loading placeholders always visible (2026-09-08) — tip **galapix-100** / bundle **galapix-011**

### Bug
After overview-aware purple policy, loading cells with no stand-in drew
**nothing** when overview was Ready (only soft overview under the grid).
Purple placeholders appeared to be "broken" again.

### Fix
Always draw purple when exact tile is missing and `find_smaller_tile` has no
stand-in: opaque if no overview surface, semi-transparent fill + outline when
overview is Ready (overview still readable, cell still marked loading).

### Status
- [x] Bundle galapix-011

---

## Pending uploads drain + note on JPEG round-trip (2026-09-08) — tip **galapix-099** / bundle **galapix-010**

### Pending uploads stuck at 10–20
`process_queue` only ran for on-screen images. Decoded tiles for briefly
visible / scrolled-off images stayed in `m_tile_queue`. Now off-screen images
with `pending_upload_count() > 0` still get `prepare_tiles` (queue drain).
Upload cap 32→64.

### Encode/decode (thumtoo-037)
Interactive path was JPEG encode (thumtoo) → store → get_tile → JPEG decode
(Galapix). thumtoo-037 delivers rgb888 in-memory and stores JPEG off the
reply path.

### Status
- [x] Off-screen upload drain
- [x] Bundle galapix-010

---

## Fix: request budget wasted on cache hits (2026-09-08) — tip **galapix-098** / bundle **galapix-009**

### Symptom
Tile debug showed mixed scale tints that never resolved to the requested
scale. Status "pending uploads" stuck around 5–20.

### Cause
`queue_tile_request` called `try_consume_request_budget()` **before** checking
whether the cell was already SUCCEEDED / in-flight. After mark_tile_needed
started recording full ancestor chains, each frame re-issued hundreds of
already-cached coarser cells first (coarser-first sort), burned the entire
global budget of 48 on no-ops, and never started jobs for missing fine tiles.

### Fix
- Check cache (and dead-job retry) first; only consume budget when starting a
  new provider job
- process_queue: 16→32 uploads/image/frame; redraw while queue non-empty even
  after a partial drain so pending uploads cannot stall when workers are idle

### Status
- [x] Budget order fix
- [x] Upload drain redraw
- [x] Bundle galapix-009

---

## Tile debug: scale-coded translucent fills (2026-09-08) — tip **galapix-097** / bundle **galapix-008**

Tile debug (`V`) draws a **semi-transparent fill + outline** whose colour
encodes the pyramid **scale of the pixels shown** (exact or upscaled stand-in).
Exact match for the requested scale gets a green bias; purple remains loading /
overview-only.

### Status
- [x] Scale palette + fill_rect overlay
- [x] Bundle galapix-008

---

## Toast notifications for toggle feedback (2026-09-08) — tip **galapix-096** / bundle **galapix-007**

On-screen ImGui toasts for short user-facing stdout messages (tile debug `V`,
tile requests `U`). Appear bottom-center for ~3s, fade out; shown even when
Tab/F1 chrome is hidden. Still printed to stdout.

### Status
- [x] ImguiOverlay::notify + draw_toasts
- [x] Wire V / U keys
- [x] Help lists `u`
- [x] Bundle galapix-007

---

## Stand-in tiles on zoom-in (2026-09-08) — tip **galapix-095** / bundle **galapix-006**

### Symptom
Zooming in showed opaque purple cells even when a lower-resolution image was
on screen a moment earlier (overview or coarser grid tiles).

### Causes
1. **Opaque purple over overview** — soft `ImageOverview` is drawn under the
   grid but missing cells filled solid purple, hiding the only available
   stand-in (common after gallery one-cell LOD which never put grid tiles in
   the tile cache).
2. **Weak mark chain** — only immediate parent + max_scale were marked; full
   ancestor chain is now marked so coarser-first issue fills stand-ins.
3. **draw_tile order** — coarser stand-in is always sought when the exact tile
   is missing (not only when all four finer children are absent).

### Fix
- `mark_tile_needed`: mark target and every coarser ancestor up to max_scale
- `draw_tile`: find_smaller first; purple only if no stand-in **and** overview
  is not Ready
- find_smaller: keep pure lookup; nearest coarser surface wins

### Status
- [x] mark chain + draw stand-in path
- [x] Bundle galapix-006

---

## Fix duplicate SDLK_r (2026-09-08) — tip **galapix-094** / bundle **galapix-005**

`R` was already bound to move/rotate tool. Tile cache-only toggle moved to **`U`**.

### Status
- [x] Use SDLK_u for tile_requests_enabled toggle
- [x] Bundle galapix-005

---

## Global mark-then-issue pass (2026-09-08) — tip **galapix-093** / bundle **galapix-004**

After the prepare/draw split, each `Image::prepare_tiles` still called
`issue_requests()` immediately, so early `m_images` entries consumed the
whole per-frame budget before later visible images could mark.

### Change
- `Image::prepare_tiles` — uploads + mark only
- `Image::issue_tile_requests` — `cache->issue_requests()`
- `Workspace::prepare_tiles` — pass 1 mark all visible, pass 2 issue all visible
- Overview `ensure_requested` still runs in prepare (may use budget for
  thumtoo overview); grid cells only after every image has marked

### Status
- [x] Split mark / issue on Image + WorkspaceItem
- [x] Workspace two-pass prepare
- [x] Bundle galapix-004

---

## Build fix: image_overview includes (2026-09-08) — tip **galapix-092** / bundle **galapix-003**

`ImageOverview::ensure_requested` (HAVE_THUMTOO path) uses `ThumtooTileProvider`
and `ImageTileCache` without including their headers. Add:

* `galapix/image_tile_cache.hpp` (budget + requests-enabled)
* `thumtoo/thumtoo_tile_provider.hpp` (under `HAVE_THUMTOO`)

Latent from the thumtoo-backed overview path; surfaced when building 002.

### Status
- [x] Includes fixed
- [x] Bundle galapix-003

---

## Tile request phase split (2026-09-08) — tip **galapix-091** / bundle **galapix-002**

Follow-up to galapix-090: implement the documented draw/update split.

### Frame structure (target)
```
Viewer::draw:
  begin_frame_request_budget(N)
  workspace.prepare_tiles(clip, zoom)  // mark + issue + uploads + overview
  workspace.draw(...)                  // pure lookup + GL
```

### API
**ImageTileCache**
- `mark_tile_needed(x,y,scale)` — records cell + stand-ins (parent, max_scale); no jobs
- `issue_requests()` — issue marked cells under budget, coarser first
- `lookup_tile(x,y,scale)` — pure map lookup; REQUESTED placeholder if absent
- `request_tile` kept as thin wrapper (mark + issue one-shot + lookup) for any residual callers
- Per-frame `m_needed` cleared at start of `issue_requests` / explicit `clear_needed`

**ImageRenderer**
- `prepare(clip, zoom)` — visibility, cancel_jobs, mark_tile_needed, overview ensure
- `draw(...)` — overview blit + lookup_tile only (no enqueue)

**Image / WorkspaceItem / Workspace**
- `prepare_tiles(clip, zoom)` virtual (default no-op on WorkspaceItem)
- Image: process overview + tile upload queue, then renderer.prepare, then issue_requests
- Workspace two-phase: prepare_tiles then draw

### Status
- [x] ImageTileCache mark / issue / lookup
- [x] ImageRenderer prepare vs draw
- [x] WorkspaceItem / Workspace / Viewer wiring
- [x] Bundle galapix-002-tile-prepare-draw-split.bundle

### Behaviour notes
- Draw path uses `lookup_tile` only (no enqueue).
- `issue_requests` sorts coarser scales first so stand-ins win the budget race.
- Per-image `prepare_tiles`: overview process + tile uploads + mark + issue.
- Key `U` still disables `queue_tile_request` (cache-only).
- Legacy `request_tile` remains as mark+issue+lookup for any residual callers.

---

## Tile request / placeholder redesign (2026-09-08) — tip **galapix-090** / bundle **galapix-001**

### Symptom
- Purple loading placeholders (`155,0,155`) stopped appearing when **no** tile
  data was ready (blank cells instead).
- Frequent **purple flashes** on tile pop-in instead of stable lower-resolution
  stand-ins (upscaled coarser cells / overview).

### Root cause analysis

`ImageRenderer::draw_tile` always calls `ImageTileCache::request_tile`, which
both **looks up** and **enqueues** (side effects on the draw path).

1. **Budget + empty return (purple gone)**  
   `queue_tile_request` returns early when the global per-frame budget
   (`begin_frame_request_budget(48)`) is exhausted and **does not insert** a
   cache entry. `request_tile` then falls through to `return SurfaceStruct()`  
   whose default `status` is `SURFACE_SUCCEEDED` (enum 0) with a null surface.  
   The draw switch only paints purple for `SURFACE_REQUESTED`. Result: no
   placeholder when the cell was never entered in the cache this frame.

2. **Flashes**  
   Under budget pressure the coarser stand-in (`max_scale` / parent) may not be
   queued in the same `request_tile` call as the target. Until something lands
   in the cache, `find_smaller_tile` (lookup-only) returns empty → purple for
   one or more frames, then the exact tile or a stand-in appears. Overview is
   drawn under the grid, but per-cell purple still flashes on top.

3. **Structural smell**  
   - Draw mutates request state and consumes a global static budget in
     traversal order (image / tile order dependent).  
   - `cancel_jobs` also runs from the draw path.  
   - Stand-in policy is ad-hoc inside `request_tile` (two opportunistic queues
     before the target).  
   - No way to freeze requesting and inspect pure in-memory cache behaviour
     (debug).  
   - Upload (`process_queue`) is correctly on the main thread; provider work is
     off-thread — good. The remaining coupling is **request issuance ↔ draw**.

### Goals for this tip
- Restore correct loading placeholders (purple) when nothing is ready.
- Prefer stand-ins over purple whenever any coarser surface exists.
- Add a keyboard toggle that **disables all new tile requests** (cache-only /
  debug mode); existing in-flight jobs still complete and upload.
- Document a cleaner long-term split (draw = pure lookup; update = batched
  issue) without a big-bang rewrite this tip.

### Short-term code changes
1. `request_tile`: if the cell is still absent after `queue_tile_request`,
   return an explicit `SurfaceStruct` with `status = SURFACE_REQUESTED` and
   null surface (do **not** leave a permanent cache entry without a job).
2. Optional small priority tweak: when budget is nearly exhausted, still try
   to queue stand-ins (max_scale / parent) preferentially — already ordered
   first; keep that order and rely on (1) for visuals.
3. Static `s_tile_requests_enabled` (default true). `queue_tile_request` and
   overview thumtoo path respect it. Key **`U`** toggles; log the state.
4. Fix stale tile-debug help text (code paints purple for loading, not
   yellow/red).

### Longer-term redesign (not all in this tip)
Desired frame structure:

```
Viewer::draw:
  begin_frame_request_budget(N)
  for each image: image.mark_visible(clip, scale)   // pure bookkeeping
  for each image: image.issue_requests()            // or one global batch
  for each image: image.process_uploads()
  for each image: image.draw()                      // pure lookup + GL
```

- `mark_visible` / desired-set: record which (scale,x,y) cells the current view
  wants; no provider calls.
- `issue_requests`: walk desired set (priority: current scale visible → parent
  → max_scale overview), start jobs up to budget; batch-friendly.
- Draw path never calls `request_tile`; only `lookup` / `find_smaller_tile` /
  placeholder by status.
- Enables true batching, stable priorities independent of draw order, and
  trivial cache-only mode.

Incremental path: keep `request_tile` as “ensure + lookup” for call-site
compatibility, but move the “ensure” part behind the enable flag and the
budget, and never lie about status on the return value (this tip).

### Status
- [x] Analysis documented (this section)
- [x] Fix empty-return status → purple restored
- [x] Cache-only toggle (`U`)
- [x] Tile-debug message corrected
- [x] Bundle galapix-001-tile-placeholder-and-cache-only.bundle (tip galapix-090)

---

## Thumtoo-backed ImageOverview (2026-09-07) — tip **galapix-089**

* Non-stdio URLs no longer mark overview Failed immediately
* ThumtooTileProvider → request max_scale (0,0) tile as overview (JPEG decode
  on worker), shares per-frame request budget
* Gallery one-cell branch: overview Ready/Loading skips draw_tile; Failed falls
  back to grid tile

---

## Restore gallery tiles; overview cannot feed rar (2026-09-07) — tip **galapix-088**

087 left fit-all blank: soft overview Fails for archive URLs and tiles were
suppressed. Restored one-cell `draw_tile`. Follow-up: load overview from
thumtoo max_scale/ladder so gallery can skip duplicate tile work later.

---

## Gallery one-tile branch: never request tiles (2026-09-07) — tip **galapix-087**

Overview Failed for rar members → previous Failed→draw_tile fallback still
issued 1064 thumtoo jobs. One-cell view scale now never calls draw_tile;
tiles only when zoomed so scaled size ≥ 256.

---

## Fix gallery LOD gate (2026-09-07) — tip **galapix-086**

screen_long*zoom LOD never tripped; gallery already used the one-tile branch
(`scaled_width < 256`) which still called `draw_tile` → 1064 requests / ~3s.

Now that branch is overview-first (wait / ready); tile only if overview Failed.

---

## Gallery LOD: overview-only under 256px on screen (2026-09-07) — tip **galapix-085**

When long edge on screen &lt; 256px:
* soft overview Ready → no tile requests
* Idle/Loading → wait (no tile stampede)
* Failed → single max_scale tile fallback

Avoids ~1064 warm `request_tile` jobs (~3s) for fit-all galleries.

---

## First-paint: 1k jobs stampeded workers (2026-09-07) — tip **galapix-084**

Measured (1064 warm-cache rar members):
* Pre-viewer ~0.15s
* Frame 1: req≈1032, upload_q≈0; pending_idle ~2.95s
* Bottleneck: provider job drain, not GL (upload_q stayed empty)

Fix: global **48 new request_tile jobs per frame**. Metrics: `ready=` =
SUCCEEDED with surface; `first_ready_surface` replaces misleading cache count.

---

## Viewer first-paint timing (2026-09-07) — tip **galapix-083**

Measured: pre-viewer open for 1064 rar members is ~0.15s (warm cache).
`viewer_session` was the whole quit latency. Now frame logs under
`GALAPIX_OPEN_TIMING` report time to first cache entry and pending_idle.

---

## Benchmark infrastructure for open path (2026-09-07) — tip **galapix-082**

* `extra/open_phase_bench` — headless phase timings (expand / size / providers /
  overview tiles), no SDL
* `GALAPIX_OPEN_TIMING=1` — phase marks in real `ViewerCommand::run`
* `BENCHMARKS.md` — how to use these vs `thumtoo-bench`

Next: optional first-frame timing inside the viewer loop once we know which
pre-viewer phase (if any) is still multi-second.

---

## Faster initial gallery fill (2026-09-07) — tip **galapix-081**

* GL uploads per image/frame: **2 → 16** (cache-hit overviews were stuck behind a tiny upload budget)
* Scale debounce: only for **adjacent** scale steps (50ms); multi-level jumps
  (open/layout/zoom-to-fit) apply immediately

---

## Keep full tile scale range in memory cache (2026-09-07) — tip **galapix-080**

`cleanup()` (off-screen each frame) used `min_keep_scale = max_scale - 2`, so
only the coarsest ~3 pyramid levels survived. Fine tiles were dropped on pan
→ entry counts stuck around ~1k for galleries.

Now: keep all SUCCEEDED tiles; only abort REQUESTED when off-screen.
No hard entry cap yet (memory grows with browsing; `clear_cache` still wipes).

---

## Remove concurrent tile-request cap (2026-09-07) — tip **galapix-079**

The REQUESTED-job cap (24 then 64) was meant to limit zoom thrash. It is
redundant with `stable_request_scale` debounce and interfered with filling
large collections. Removed.

---

## Revert small-on-screen LOD early-out (2026-09-07) — tip **galapix-078**

The `< 192px` overview-only path used `image_rect * zoom`, which does not
match how on-screen size is computed in this renderer. Result: often skipped
the tile grid while overview was empty → black / intermittent draws.

Keep per-image request cap at 64. Any future LOD must use the same
geometry as `tiledb_scale` / vertices and still request at least overview
tiles when soft overview is missing.

---

## Per-image request cap 64 + small-on-screen LOD (2026-09-07) — tip **galapix-077**

* Concurrent REQUESTED cap is **per ImageTileCache**, not global. 300 pending
  across a gallery is many images each with a few jobs.
* Cap raised **24 → 64** so one deep-zoomed image can fill a ~full-HD grid.
* If an image is **&lt; 192px** on screen (long edge), skip the tile grid entirely
  (overview only). Cuts draw calls and provider spam for hundreds of thumbnails.

### Draw calls (note)
N visible images ⇒ ≥N textured draws (overview). Plus one draw per visible
tile cell when zoomed in. ~1000 thumbnails ≈ ~1000 draws — fine for modern GL
if each is a single overview quad; the problem is N×(tile grid) when every
thumbnail also requested a multi-tile pyramid. LOD above prevents that.

---

## Zoom CPU spikes: debounce scale + cap in-flight jobs (2026-09-07) — tip **galapix-076**

### Cause
Fast zoom steps `tiledb_scale` every frame. Each step:
* enqueues a full visible grid of `request_tile` (PDF raster / JPEG on workers)
* `cancel_jobs` marks handles aborted, but **thumtoo cannot stop mid-job**
* workers finish discarded work → CPU spikes and uneven frame times

### Fix
* **`stable_request_scale`**: hold the previous request scale until the desired
  scale is unchanged for **80ms** (continuous zoom never commits intermediates)
* **Cap concurrent REQUESTED** at 24 (overview always allowed)

---

## Main loop: vsync instead of SDL_Delay(10) (2026-09-07) — tip **galapix-075**

### Why 60fps was impossible
`AppViewer::run` ended every frame with **`SDL_Delay(10)`**. At 60Hz the
frame budget is ~16.7ms; sleeping 10ms left ~6ms for tiles/GL/ImGui. Any
upload work pushed the total over a frame → visible jank, never a steady 60.

### Fix
* `SDL_GL_SetSwapInterval(1)` after window create (pace on display refresh)
* Remove the fixed 10ms delay
* `trigger_redraw`: single `SDL_PushEvent` (no busy-wait if queue full)

---

## Frame-time: stop draw-path parent fan-out (2026-09-07) — tip **galapix-074**

Measured serious GUI work while zooming (after thumtoo-035 moved I/O off GUI):

1. **`find_smaller_tile`** enqueued **every** coarser scale for each missing
   visible cell (`O(max_scale × visible_tiles)` provider jobs + map inserts
   per frame). Now **lookup-only**; `request_tile` already queues parent +
   overview.
2. **`cancel_jobs`** walked the full REQUESTED map every draw even when the
   visible rect/scale was unchanged. Skip when args match last call.
3. Cap GL uploads at **2**/image/frame (was 4) to leave more headroom.

Not touched (not the main frame-time spike once above is fixed): worker decode,
receive_tile queue depth.

---

## Frame drops on zoom — thumtoo request_tile (2026-09-07)

Root cause was **thumtoo** `Client::request_tile` synchronous `get_tile` on the
caller (GUI) plus inline Executor decode. Fixed in **thumtoo-035** (always
enqueue). Rebuild Galapix against that thumtoo tip.

Remaining Galapix-side costs (follow-ups if still janky):
* `cancel_jobs` O(cache) every draw while zooming
* `find_smaller_tile` queues many parent requests per visible cell
* GL upload cap is already 4/frame in `process_queue`

---

## More transitive-include fixes (2026-09-07) — tip **galapix-072**

* `viewer_command.cpp`: `util/filesystem.hpp`
* `tile_generator.hpp`: `math/size.hpp`, `util/url.hpp`

---

## Fix includes after ResourceDatabase removal (2026-09-07) — tip **galapix-071**

`Size` / `Math` were previously included transitively via database headers.
Add direct includes in `tile_provider.hpp` and Mandelbrot translation units.

---

## CMake: drop PkgConfig::SQLITE3 from libgalapix (2026-09-07) — tip **galapix-070**

Flake provides sqlite for thumtoo’s own CMake; Galapix must not link
`PkgConfig::SQLITE3` (find was removed with ResourceDatabase).

---

## flake: keep sqlite for thumtoo (2026-09-07) — tip **galapix-069**

Removing Galapix SQLiteCpp dropped `sqlite` from flake `buildInputs`, but
thumtoo’s CMake still does `pkg_check_modules(... sqlite3)` (line 16). Restore
`sqlite` under the thumtoo dependency block. Galapix sources do not use it.

---

## Remove ResourceDatabase / cache4 resource index (2026-09-07) — tip **galapix-068**

### Removed from Galapix
* `Database` / `ResourceDatabase` and all `src/database/**`
* `src/sqlite/**`, `src/resource/**`, `src/generator/**` (idle, only served DB)
* `util/sha1` + OpenSSL Crypto link (only used for cache4 blob ids)
* SQLiteCpp / sqlite3 package deps from CMake and flake
* ViewerCommand no longer opens `~/.galapix/cache4`
* `-d/--database` kept as **deprecated no-op** (warns) for old scripts

### Not removed
* Workspace `.galapix` files (layout) — independent of cache4
* arxpcpp directory scan (`Filesystem` / `App::archive`)
* Overview / `TileGenerator` (stdio soft preview)
* Zoomify / Mandelbrot

### Unfinished Galapix features → thumtoo TODO
Documented in **thumtoo** tip notes (thumtoo-034): archive passwords, richer
video meta, HTTP content-type/validators, avoid resurrecting handler graph.

On-disk `~/.galapix/cache4.sqlite3` is orphaned; safe to delete manually.
No automatic migration (view path already used thumtoo only).

---

## Remove idle DatabaseThread + tile stubs (2026-09-07) — tip **galapix-067**

* Dropped `DatabaseThread` from `ViewerCommand` (was start/join only).
* Deleted `database_thread.*`, `file_entry_generation_job.*`,
  `cached_tile_database.*`, `memory_tile_database.*`, `tile_database_interface.hpp`.
* `Database` is resource-only (still opens `cache4.sqlite3`; view path does not read it).
* `TileEntry` kept for `Tile` ctor compatibility; not a storage path.

Next: consider not constructing `Database` at all under HAVE_THUMTOO, then
SQLiteCpp / ResourceDatabase deletion.

---

## View path ignores ResourceDatabase (2026-09-07) — tip **galapix-066**

### Concern
Does `ResourceDatabase` still hold information thumtoo does not cover?

### Answer (view path)
**No.** The open path only ever needed URL + pixel size for
`ThumtooTileProvider`. `make_file_tile_provider` already ignored
`OldFileEntry` / `ImageEntry` under `HAVE_THUMTOO`. Coverage matrix:

| ResourceDatabase | thumtoo |
|------------------|---------|
| path, mtime | locators |
| sha1 blob id | content_id (sha256) |
| image WxH | content / get_size |
| archive TOC | archive_entries |
| URL rows | locators + HTTP |
| video duration | duration_ms / stills (partial) |
| archive password | **not covered** (also unused by view) |

Workspace `.galapix` files are independent of cache4.

### Change
* Open path always uses `make_file_tile_provider(url)` — no
  `get_old_file_entry` / `get_image_entry`.
* Fixes: file row present but missing `ImageEntry` no longer drops the image.
* `Database` / `DatabaseThread` still constructed (schema + idle thread) for a
  later deletion pass; not read for viewing.

See [docs/CACHE4_VS_THUMTOO.md](docs/CACHE4_VS_THUMTOO.md) Phase 3 matrix.

---

## Obsolete-cruft audit after thumtoo (2026-09-07) — tip **galapix-065**

### Already gone (Phases 0–2)
`cache4_tiles.sqlite3`, `SQLiteTileDatabase`, `DatabaseTileProvider`,
`TileGenerationJob` / `MultipleTileGenerationJob`, tile SQL statements/tables,
`FileTileDatabase`. CMake no longer pulls mhash/jsoncpp/EnTT/Python.

### Applied this tip
* flake: remove residual **entt**, **python3**, **libexif** from `buildInputs`
* docs: DEPENDENCIES + CACHE4 Phase 3 inventory of **idle** vs **still live** code
* options.hpp comment: no legacy SQLite tile path when `HAVE_THUMTOO`

### Still live (do not delete yet)
| Piece | Why |
|-------|-----|
| `cache4.sqlite3` / `ResourceDatabase` | Open path `get_old_file_entry`; optional size cache |
| `TileGenerator` + `OverviewLoadJob` | Soft stdio overview under tiles |
| Zoomify / Mandelbrot providers | Non-thumtoo sources |
| arxpcpp via `Filesystem` / `App::archive` | Directory scan of archives |
| SQLiteCpp, Magick, curl, OpenSSL | Resource DB / loaders / network / SHA1 |

### Idle call graph (next deletion candidates)
| Piece | Evidence |
|-------|----------|
| `DatabaseThread` | Only `start`/`abort`/`join` from `ViewerCommand`; no `request_*` callers |
| `FileEntryGenerationJob` | Only reached from that thread; still cuts Galapix tiles into resource DB |
| `MemoryTileDatabase` / `CachedTileDatabase` | No view-path readers; only `delete_file_entry` → `delete_tiles` |
| `ArchiveThread` | No external request callers |
| `src/generator/`, most of `src/resource/` | Not on thumtoo view path |

### Recommended next commits (small, ordered)
1. Stop constructing/starting `DatabaseThread` when `HAVE_THUMTOO` (or entirely).
2. Open path: always thumtoo provider when `HAVE_THUMTOO` (skip resource probe).
3. Delete tile-stub classes + `FileEntryGenerationJob` if still unreferenced.
4. Later: arxp / resource DB / SQLiteCpp once scans and metadata move fully to thumtoo.

Do **not** remove Magick/curl/arxp in the same pass as (1)–(3); separate product decisions.

---

## PDF live zoom floor raised to scale −8 (2026-09-07) — tip **galapix-064**

`ThumtooTileProvider` clamped PDF `min_scale` at **−4** (≈2304 dpi). Past that
ImageRenderer kept requesting scale −4 and upscaled tiles — hard ceiling around
“one tile ≈ one glyph”.

| scale | dpi (layout 144) |
|------:|-----------------:|
| 0 | 144 |
| −2 | 576 (durable floor) |
| −4 | 2304 (old live floor) |
| −6 | 9216 |
| −8 | 36864 (new live floor) |

thumtoo region path has no DPI clamp (full-page fallback still clamps 2400).
Live memory is O(tile). Change: `kPdfMinLiveTileScale = -8`.

---

## Live PDF tiles investigation (2026-09-07) — tip **galapix-063**

### Symptom
Cached PDF tiles (JPEG from thumtoo durable store) render. Live /
interactive cells — especially **negative scale** and first-miss path that
returns **`codec=rgb888`** — stay purple or never refine.

### Already correct at tip `c7f4975` (+ thumtoo `32bb12a`)

| Piece | Status |
|-------|--------|
| `surface_from_tile_blob` | Branches on `codec == "rgb888"` → RGBA8; else JPEG |
| `get_min_scale()` | PDF `//page:` → **−8** (was −4; see tip 064) |
| ImageRenderer scale | `floor(log2(1/(zoom·s)))` + `ldexp` for scale_factor &lt; 1 |
| Tile grid math | `itilesize = 256 · 2^{scale}` matches thumtoo region geometry |
| flake.lock thumtoo | Pins tip with live rgb888 + durable floor −2 |

thumtoo side (verified separately): interactive PDF **always** replies
`rgb888`; durable JPEG only for `scale ≥ −2`; geometry tests pass.

### Root cause found (Galapix cache)

`ImageTileCache::queue_tile_request` only re-queued cells whose handle was
**`is_failed()`**. Live PDF region renders are slow; `cancel_jobs` often
**`set_aborted()`** when the user pans/zooms before the worker finishes.

Race:

1. Request scale −1 cell → entry `SURFACE_REQUESTED`
2. Worker still in Poppler region render
3. View moves → `cancel_jobs` may **abort without erase** in some paths, or
   the worker completes after an abort flag is set while the entry remains
4. `deliver` sees `is_aborted()` and returns **without** `set_failed()`
5. Entry stays `REQUESTED`, no surface, **not** `is_failed()` → never retried
6. Permanent purple for live-only / negative-scale cells

Cached JPEG hits complete quickly on the first try → rarely hit the race.

### Fix (this tip)

* Treat **`is_aborted()` like `is_failed()`** for retry (up to 3 attempts).
* Richer decode-failure log (codec, byte size, meta WxH).

### Still watch

* After 3 dead attempts the cell stays dead until image reload — consider
  periodic retry or clear-on-zoom-in later.
* Overview still skips non-stdio URLs (`//page:` has plugin) — intentional;
  grid tiles are the only PDF preview.
* Live floor is now −8 (tip 064); durable store still only ≥ −2 in thumtoo.

---

## get_min_scale for PDF negative tiles (2026-09-07)

Not a boolean flag: **`TileProvider::get_min_scale()`** (default 0) is the
range bound the cache/renderer need. PDF via thumtoo: min_scale = **−8**.
ImageRenderer uses float `ldexp` for scale_factor so scale < 0 works.

Pairs with thumtoo-030 region PDF tiles + live rgb888.

## Fix: -p batch size probe (2026-09-07)

Large `-p` lists looked stuck near the end of progress: each size miss called
`ThumtooTileProvider::create` → `request_size` + `drain` (sequential I/O).
Now batch `request_size` once, then `create_from_size` only. Progress counts
processed locators, not only successful adds.

## Implement `-p` / `--pattern` via thumtoo locator queries (2026-09-07)

**Goal:** Replace the dead/stub resource-DB pattern path with thumtoo
`list_locators_by_uri_prefix` / `list_locators_by_outer_path_prefix` /
`list_locators_like` (thumtoo-026 / commit ca14b71+).

### Plan (galapix-059)

1. **Bump thumtoo flake input** to a tip that includes locator query APIs
   (ca14b71 or later; currently locked at 29d1b4f). Without nix in the
   agent environment, edit `flake.lock` manually or document
   `nix flake lock --update-input thumtoo` for the packager. Local builds
   can use `-DTHUMTOO_DIR=/path/to/thumtoo` at tip.

2. **`url_from_thumtoo_uri`** in `src/thumtoo/thumtoo_uri.{hpp,cpp}`:
   reverse of `thumtoo_uri_from_url` using `thumtoo::parse_location`.
   Map `file://` + optional `//archive:` / `//page:` pipes to Galapix URL
   form; pass through `http(s)`.

3. **`ViewerCommand` pattern block** (`viewer_command.cpp`):
   - When `HAVE_THUMTOO` and `m_thumtoo`:
     - `*` → `list_locators` (high limit, e.g. 50_000)
     - absolute path / `file://` prefix → `list_locators_by_outer_path_prefix`
     - otherwise → convert legacy GLOB `*`/`?` to SQL LIKE `%`/`_` and call
       `list_locators_like` (also accept raw `%`/`_`)
   - For each `LocatorRow`, `url_from_thumtoo_uri` → `make_file_tile_provider`
     → `workspace.add_image`
   - Keep resource-DB stubs as no-op fallback when no thumtoo (already empty)

4. **CLI help** (`arg_parser.cpp`): update `-p` text from “legacy; query →
   thumtoo later” to describe cache-only thumtoo match (LIKE / path prefix).

5. **Docs:** `docs/THUMTOO.md`, `docs/CACHE4_VS_THUMTOO.md` — mark `-p` as
   thumtoo-backed; resource DB no longer required for pattern open.

6. **Limits:** start with limit 50_000; log if truncated. No new CLI flag
   unless needed later.

7. **Commits (small, focused):**
   - docs: plan for thumtoo-backed `-p`
   - thumtoo_uri: reverse mapping
   - viewer: implement pattern via Client list APIs
   - help + docs update
   - handoff tip galapix-059

Author: Ingo Ruhnke `<grumbel@gmail.com>` + `Co-authored-by: Grok <grok@x.ai>`.

### Status

* [ ] flake / THUMTOO_DIR tip with query APIs (agent env had no nix; use local THUMTOO_DIR or `nix flake lock --update-input thumtoo`)
* [x] url_from_thumtoo_uri
* [x] ViewerCommand pattern path
* [x] help + docs
* [x] bundle galapix-059
* [x] fix namespace for url_from_thumtoo_uri (060)

## Session handoff (2026-09-07) — tip **galapix-062** / thumtoo **026+**

### Tip bundles (apply latest; stack cleanly)

| Project | Tip bundle | Tip commit (subject) |
|---------|------------|----------------------|
| **galapix** | **`galapix-062.bundle`** | `get_min_scale` + PDF negative tile zoom |
| **thumtoo** | tip with **ca14b71+** (locator prefix/LIKE) | query APIs for Galapix `-p` |

Author for commits: Ingo Ruhnke `<grumbel@gmail.com>` with trailer  
`Co-authored-by: Grok <grok@x.ai>`. Bundles use **HEAD** as ref, full history from first checkout, continuously numbered.

### Galapix changes this session (high level)

**UI (earlier tips ~036–041 era, still in history)**  
* Split Status / Help; Help toolbar icon; clickable shortcut rows  
* Keyboard: only block on `WantTextInput`; disable ImGui nav keyboard capture  

**cache4 tile retirement**  
* Phase 0–1: no `cache4_tiles` when `HAVE_THUMTOO`; dead deps/docs  
* **Phase 2 (043+)**: removed `SQLiteTileDatabase`, tile statements/table,  
  `DatabaseTileProvider`, `TileGenerationJob` / multi-tile jobs, DatabaseThread tile APIs  
* `Database::create(prefix)` → resource `cache4.sqlite3` + in-memory tile stub only  
* `TileGenerator` **kept** for overview JPEG scale path  
* Build fixes: `RowId` include, `OldFileEntry` in DatabaseThread, drop dead includes in `image.cpp`  

**Dead code**  
* Removed `ResourceManager`, idle `DownloadManager` in `galapix.cpp`  
* Still compile / unused on viewer path: `Generator`, `BlobManager`, much of `src/resource/*`  
  (types still used by ResourceDatabase — trim carefully)  

**thumtoo integration**  
* `thumtoo_uri_from_url`: **http/https pass-through** (053) for remote images  
* File / archive / PDF page mapping unchanged  
* Docs: `docs/THUMTOO.md`, `docs/CACHE4_VS_THUMTOO.md` product direction  
* **Next:** `-p` via thumtoo locator list APIs (see plan above)

### Architecture (viewer path) — current

```
CLI (files / http(s) URLs)
  → ViewerCommand
      → Database (resource cache4.sqlite3 only; no cache4_tiles)
      → thumtoo::Client when HAVE_THUMTOO
      → Workspace + Image(TileProvider)
          → ThumtooTileProvider | ZoomifyTileProvider | MandelbrotTileProvider
  → Viewer (SDL) + ImGui overlay (Status / Help)
```

**Not** used for file tiles: SQLite tile DB, DatabaseTileProvider (deleted).

### Thumtoo dependency (pair with galapix)

Prefer **thumtoo tip with locator query** (ca14b71+ / outer_path index).  
Needs **libcurl** in the environment so nested thumtoo build gets `THUMTOO_HAVE_CURL`.  
Galapix flake already lists `curl` in build inputs; **update flake input** to
include query APIs when publishing.

### Explicitly deferred

* Galapix “dataverse” UI (much later); consume thumtoo, do not revive cache4 tiles  
* Tags / collections query (thumtoo still open)  
* Pure no-store live PDF; per-tile PDF region render  

### Next suggested work

0. **Implement `-p` via thumtoo list APIs** (this tip / galapix-059)
1. Smoke: local file, PDF page, archive member, `http(s)://` image  
2. Point galapix `flake` `thumtoo` input at published tip with query + curl  
3. Optional: delete/gate `Generator` / `BlobManager` if still unreferenced  

### Doc map

| File | Role |
|------|------|
| [TODO.md](TODO.md) | This handoff + historical backlog |
| [docs/THUMTOO.md](docs/THUMTOO.md) | Flags, URI/http/PDF notes |
| [docs/CACHE4_VS_THUMTOO.md](docs/CACHE4_VS_THUMTOO.md) | Phase 0–2 matrix; product horizons |
| [AGENTS.md](AGENTS.md) | Agent rules + architecture |
| thumtoo TODO | Retrieval API bundle table (016–023) |


## thumtoo-022 HTTP session body cache (2026-09-07)

* In-process 512 MiB cache for successful GETs

## Galapix http(s) → thumtoo (2026-09-07)

* `thumtoo_uri_from_url` passes http/https through for remote images

## thumtoo-021 HTTP(S) fetch (2026-09-07)

* Optional libcurl; probe/tiles/pixels/read_source_bytes for http(s)

## thumtoo-019 PDF live tiles @ 144 dpi (2026-09-07)

* Page size and request_tile rasterize at kPdfLayoutDpi

## thumtoo-018 read_source_bytes (2026-09-07)

* Source bytes by location or content-id (file/archive); not PDF/http yet

## thumtoo-016 Location URI API (2026-09-07)

* Apply **thumtoo-016.bundle** (or newer) for parse_location / list_locators
* Galapix can adopt helpers gradually; no forced API break this session

## Dead code candidates after tile removal (2026-09-07)

* Removed unused `DownloadManager` construction from `galapix.cpp` (Zoomify uses
  `network/curl.hpp` directly)
* Deleted never-wired `ResourceManager` (old tile/resource bridge)
* Still compile but unused from the viewer path: `Generator`, `BlobManager`,
  most of `src/resource/*` except types still used by ResourceDatabase —
  trim carefully later; long-term retrieval → thumtoo

## thumtoo direction: retrieval URIs + live PDF (2026-09-07)

* **Not Galapix work now** — document only; implement in thumtoo when ready
* Flexible nested URIs: network, archives, nested archives, PDF pages, sha1/sha256 blobs
* Blob/network access currently split across Galapix (curl/arxp) — long-term **thumtoo**
* Live PDF → tiles at requested scale (mandelbrot-style), not only fixed-dpi cache
* See [docs/THUMTOO.md](docs/THUMTOO.md)

## Phase 2 done: Galapix SQLite tile stack removed (2026-09-07)

* Deleted `cache4_tiles` path, `SQLiteTileDatabase`, tile SQL statements/table,
  `DatabaseTileProvider`, tile generation jobs, DatabaseThread tile APIs
* Resource `cache4.sqlite3` remains for `-p` until thumtoo query exists
* `TileGenerator` kept for overview JPEG scale path

## Phase 1 done: HAVE_THUMTOO never uses cache4_tiles (2026-09-07)

* `--no-thumtoo` rejected; always `sqlite_tiles=false` when HAVE_THUMTOO
* Next Phase 2: delete SQLite tile sources from default build / tree (still
  needed only for WITH_THUMTOO=OFF)
* Query/library → thumtoo; dataverse UI much later

## Phase 1: deprecate Galapix SQLite tiles (2026-09-07)

* `--no-thumtoo` → deprecation warning (HAVE_THUMTOO builds)
* Product: library **query** → thumtoo later; Galapix stays viewer + workspace
* **Dataverse** (browsable corpus) = much later; do not design cache4 for it
* Phase 2 **done**: SQLite tile stack deleted (see Phase 2 note above)
* See [docs/CACHE4_VS_THUMTOO.md](docs/CACHE4_VS_THUMTOO.md)

## cache4 vs thumtoo audit (2026-09-07)

* Full matrix + removal phases: [docs/CACHE4_VS_THUMTOO.md](docs/CACHE4_VS_THUMTOO.md)
* **Tiles:** Galapix SQLite tile stack removed; file tiles need thumtoo
* **Resource** (`cache4.sqlite3`): still required for `-p` / file index
* Phase 0 done: deleted dead `FileTileDatabase`; dropped unused libmhash, jsoncpp, EnTT, Python find
* Next: deprecate `--no-thumtoo` after UI smoke; then Phase 2 delete SQLite tile stack
* Do **not** delete resource DB in the same change as tiles

## Keyboard shortcuts vs ImGui focus (2026-09-07)

* Bug: after clicking toolbar/Help, Viewer keys (p/z/1–6/…) stopped working
  until the canvas was clicked again (`WantCaptureKeyboard` stayed true).
* Fix: do not use `NavEnableKeyboard`; only swallow keys when `WantTextInput`
  (text fields). Mouse capture unchanged so chrome still blocks pan/zoom.

## Help panel + clickable shortcuts (2026-09-07)

* Split former **Status / Help** into two panels:
  - **Status** (toolbar icon): view scale, tile backlog, overview stats only
  - **Help / Shortcuts** (new toolbar icon `help.svg`): categorized key list
* Help rows that map to `Viewer` methods are **clickable** (run the action)
* Non-clickable: Tab/F1 chrome, F11 fullscreen, F12 screenshot, Esc quit, mouse bindings
* Docs: [docs/STATUS_REPORTING.md](docs/STATUS_REPORTING.md)

## ImGui tools + hide chrome (2026-09-07)

* SVG sources in `data/icons/svg/`; PNGs for ImGui textures
* Tools window: pan/zoom/grid/move, grid toggles, layouts
* **Tab** or **F1** toggles all ImGui chrome (H stays zoom-home)


## GTK removed (2026-09-07) — SDL only

* Deleted `src/gtk/`, `src/main/gtk_main.cpp`, `data/gtk/`
* CMake: no `BUILD_GALAPIX_GTK` / gtkmm; flake: no `galapix_gtk` app, no gtkmm3
* **Kept** `data/icons/hicolor/24x24/actions/*.png` (toolbar glyphs from the old UI)
* Next: repaint as SVG sources, optional PNG at build time, wire into ImGui tool chrome

Galapix ToDo
============


## Merge master (v0.2.2) → develop → master (2026-09-07)

History diverged for years (~675 develop-only commits; master still Boost/GLEW/SCons→CMake line).
A content merge of master into develop is not viable.

**Strategy:**
1. Port useful master-only changes onto develop (controller tweaks from c709bb83).
2. `git merge -s ours origin/master` on develop — records master ancestry, **keeps develop tree**.
3. Merge develop into master (fast-forward or normal merge) so master tip == develop.
4. Delete `develop` branch; single line of history on `master`.
5. Tag later as 0.3.0 when ready (VERSION remains 0.3.0-dev until then).

Master v0.2.2 is the last public Boost-era / SDL-only Nix line.

## Priority: merge develop → master

Make **develop** the branch that becomes **master**. See
[docs/DEVELOP_VS_MASTER.md](docs/DEVELOP_VS_MASTER.md).

### Policy

* **GTK frontend removed** — single binary `galapix` (SDL + ImGui). All new UI is SDL.
* **thumtoo local changes:** if patching is needed again, **vendor with
  `git subtree`** (not floating `patches/*.patch`); upstream in larger
  batches. See AGENTS.md.


* Master-only CLI (`prepare`, `filegen`, `check`, `merge`) → **not** ported;
  use **thumtoo-prepare** and improve thumtoo UX for the rest.
* cache3 → cache4: **feature parity** first; optional **manual** conversion
  tool later (not a merge blocker).
* Highest concern: **UI differences / regressions** (interactive testing).

### Checklist

* [ ] UI smoke: zoom scales, edge tiles, window resize
* [x] UI: F11 fullscreen **toggle** (SDL; leave with F11 again)
* [ ] UI: tools, layouts 1–6, sort/shuffle, isolate, workspace F2/F3
* [x] NEWS: cache3/cache4 incompatibility (no auto migrator required)
* [x] NEWS: architecture (wstdisplay/surfcpp/thumtoo; no Boost/GLEW)
* [ ] Merge develop → master (or retarget default branch after tag)

## thumtoo integration



* [x] Optional ThumtooTileProvider (WITH_THUMTOO / THUMTOO_DIR)
* [x] Wire provider into image open path (`--thumtoo`)
* [x] Session default cache root ($XDG_CACHE_HOME/thumtoo)
* [x] Prefetch overview + request coarser stand-ins in find_smaller_tile
      (reduce purple loading tiles on fast zoom)
* [x] Fix bad_weak_ptr: prefetch_overview after make_shared (not in ctor)
* [x] Expand PDF pages + archive members on open; sync desktop MIME with thumtoo
* [x] Fix viewer_command expand (string literal + no format.hpp dependency)
* [x] Flake: enable WITH_THUMTOO against thumtoo (tile API commit)
* [x] Align size probe with Client::drain / get_size after request_size
* [x] Default-on when HAVE_THUMTOO; `--no-thumtoo` forces SQLite tiles
* [x] Pure thumtoo view: no cache4_tiles.sqlite3, no DatabaseTileProvider fallback
* [x] GUI-thread Executor (ThumtooCallbackQueue pumped from Viewer::draw)
* [x] Tile UV half-texel inset (LINEAR seam / black border)
* [x] Offload tile JPEG decode off GUI; limit GL uploads per frame
* [x] Batch thumtoo size probes at view open (Processing URLs)
* [x] Tile load policy (partial): no interactive full pyramid; overview ≠ tiles; stand-in + cancel policy
* [x] thumtoo: single-scale on request_tile (upstream; not full pyramid)
* [ ] thumtoo: single-cell cut (upstream still open)
* [x] thumtoo: same-archive extract coalesce + member cache (upstream thumtoo-005+)
* [x] Request coarser stand-in on tile miss (overview + parent; cancel keeps coarser)
* [x] Fast overview path (libjpeg scale) with separate in-memory layer — [docs/OVERVIEW.md](docs/OVERVIEW.md)
* [ ] EXIF thumbnail path for overview (optional refinement)
* [ ] Benchmark: split vs merged size+overview for archive members
* [ ] Optional: texture array / atlas batching for fewer draw calls
* [x] Console tile load stats (`l` / `print_state`: pending requests, uploads, cache entries, thumtoo callback queue)
* [x] On-screen status overlay (Dear ImGui, F1; see [docs/STATUS_REPORTING.md](docs/STATUS_REPORTING.md))

### Next implementation targets (tile / archive)

1. **EXIF embedded thumbnail** for overview (optional; libjpeg scale covers JPEG files).
2. **Upstream thumtoo**: single-cell `request_tile` cut + same-archive member
   request coalesce (Galapix cannot fix libarchive seek pain alone).
3. Optional: surface `thumtoo::Client` job backlog once Client exposes stats.
4. Benchmark size-only vs size+overview on archive members before locking the
   open pipeline.
5. Expand ImGui chrome (tools/workspace panels) without putting logic in widgets.

In-memory overview (stdio) + max_scale grid stand-in are in place.


Galapix ToDo
============

Release procedure
-----------------

* test that Galapix works with and without SpaceNavigator libraries

* test that all Galapix functions are accessible without SpaceNavigator

* test that mouse wheel is supported

* test svn trunk a lot

* test database compatibilty

* increment the version number

* check that all keyboard shortcuts are documented and sort them a bit to be logical

* svn cp https://galapix.googlecode.com/svn/trunk/galapix@${NUM} tags/galapix-0.1.2

* svn export http://galapix.googlecode.com/svn/tags/galapix-0.1.2


Galapix 0.2.0 Release ToDo
--------------------------

* check with sqlite3_table_column_metadata() that the table has the
  right format

* need to display the thread status graphically somehow (graphical
  rotating wheel? per image or overall?)

* seperate galapix from the thumbgen code, so that there is a
  galapix-tool and a galapix program

* galapix FILE...
   -h, --help
   -d, --database FILE    Use database FILE
   -p, --pattern PATTERN  Show all files in the database matching PATTERN

* galapix-thumbgen
   -m, --minsize X      Generate tiles till this size
   -d, --database FILE

* galapix-db [info|list|merge|cleanup|delete]
   -d, --database FILE

  galapix-db merge -d outdatabase.sqlite indatabase1.sqlite indatabase2.sqlite ...
  galapix-db cleanup
  galapix-db delete -d database1.sqlite [PATTERN]...


Bugs and Issues
---------------

* "libpng error: Not a PNG file" should not leak out of libpng, should
  be handled by proper logging functions

* loading failures need to be communicated to avoid rerequests

* mtime needs to be checked

* images belonging to an archive/directory need to be groupable

http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112


Galapix 0.1.3 Release ToDo
--------------------------

User Interface
--------------

* add view-limiter that limits scrolling to the images on the screen

* refresh fails at high zoom levels (displays wrong tiles)

* rewrite overlap solving to be fast or disable it, as it currently
  just halts the whole app

* fix random layout so that all image sizes are handled equally,
  currently low-res images get really small, also take aspect
  ration into account

* add function to relayout just the selection instead of all images

* make middle button exit trackball mode

* ZoomTool doesn't know about trackball_mode and thus zooms not into
  the center of the picture, but to the mouse cursor


Other Things
------------

* always flush database completly in one go (whey do we get multi
  flushes in a row?)

* io kills interactivity (why?)

* (!!!) jpeg's from archive should only generate tiles fro their zoom level,
  generating all kills performance and eats tons of memory (why?)

* we might need a ThreadMessageQueue with a size limit, that blocks when it is full

* create a directory with test images for all supported image and
  archive formats

* double-load of .kra files is still present

  - FileEntry is generated -> send to image
  - Tiles start generating
  - image request tiles
  - new TileGeneration job is started as FileEntryGenerationJob isn't tracked

  Solution: either add tracking of FileEntryGenerationJob or merge it
  back into TileGenerationJob

* add more custom sorter (sort by date, ...)

* need to handle mtime properly (checking is expensive for thousands
  of images), currently mtime is recorded, but not checked on load, at
  least for some files (archives might differ)

* sorting needs to be changed, as a TileProvider doesn't necesarrily
  have a URL (pass URL and TileProvider to Image? Have a
  TileProvider::get_url()?)

* broken images must be detected and marked, so they aren't
  re-requested over and over again

* make thumbnail generation output optional --log all --verbose == --log info

* need to find proper location to filter out Size(0,0) FileEntries


General Problems
----------------

* currently only out-of-screen cancels jobs, but zooming need to cancel jobs too

* need a framework do handle downloads in a multithreaded fashion

* add other gigapixel format support

http://gasi.ch/blog/openzoom-description-format/

http://gigapan.org/gigapans/15374/
var fl_vars = {
 url:"http://tile15.gigapan.org/gigapans0/15374/tiles/",
 suffix:".jpg",
 startHideControls:"0",
 width:"59783",
 height:"24658",
 nlevels:"9",
 cleft:"0",
 ctop:"0",
 cright:"59783.0",
 cbottom:"24658.0",
 startEnabled:"1",
 notifyWhenLoaded:"1",
 startHideWatermark:"1"
};

1 http://tile15.gigapan.org/gigapans0/15374/tiles/r0.jpg
2 http://tile15.gigapan.org/gigapans0/15374/tiles/r00.jpg
3 http://tile15.gigapan.org/gigapans0/15374/tiles/r00/r000.jpg
4 http://tile15.gigapan.org/gigapans0/15374/tiles/r00/r0000.jpg
5 http://tile15.gigapan.org/gigapans0/15374/tiles/r00/000/r00000.jpg
6 http://tile15.gigapan.org/gigapans0/15374/tiles/r00/000/r000000.jpg
7 http://tile15.gigapan.org/gigapans0/15374/tiles/r00/000/r0000000.jpg
8 http://tile15.gigapan.org/gigapans0/15374/tiles/r00/000/r00000000.jpg

- http://tile15.gigapan.org/gigapans0/15374/tiles/r00/000/000/r000000000.jpg

* a separate table for small tiles could speed things up (i.e. either
  just the lowres'est tile or all tiles that contain the whole image)

select count(*) from tiles;
5695025
select count(*) from files;
500126

* including all fields of the tiles table in the tiles_index could
  speed things up 2x, but would require 2x the storage

* sometimes images end up tiny, some race condition in getting image size likely

* deletion of tiles/fileentries is *extremely* slow, maybe grouping could help

* do something to ensure that callbacks always go back to a valid
  object, not an already deleted one

* temp mousegrab while middle button is held for better scrolling

* review the API of SoftwareSurface and clean it up,
  shared_ptr_from_this() could be avoided when static functions are
  used instead of member functions, i.e.

  SoftwareSurfaceFactory::transform(surface, kRot90);
  SoftwareSurfaceFactory::clone(surface);

* very small images seem broken

 TileGenerationJob: have [0..1] generating [-1..-1]: FileId(51152):

* separate animation from Image, maybe even position

    galapix.sdl: src/jobs/tile_generation_job.cpp:54: bool TileGenerationJob::request_tile(const JobHandle&, int, const Vector2i&, const boost::function<void(TileEntry)>&): Assertion `scale < m_min_scale_in_db || scale > m_max_scale_in_db' failed.

* failed loading does not propagate to the image, need some kind of
  callback to report errors

* requests can stack up so that galapix is busy flushing the cache
  multiple times in a row before stuff can continue

* TileEntry::get_blob vs TileEntry::get_surface who uses what?

* figure out where all the memory goes, galapix should ideally need no
  more then what is visible on the screen, which is much less then the
  hundreds of megabyte that ps shows

* memory behaviour:

  after start up: 50MB
  when first picture appear on screen instant jump to 128MB
  low zoom levels don't seem to wast space, only high ones
  zooming in and out also wastes space

* make tile and file cache size configurable

* DatabaseThread is very similar to JobWorkerThread, maybe merge them

* maybe have something like JobHandle->spawn_child_job() -> No

* quality of small thumbnails is very bad, should start with larger
  thumbnails and then halve() them down instead of using scale()

* shared_ptr<> impl is evil

* implement some proper stuff to report the current status (what jobs
  are running, what an image is doing, etc)

* add a is_queued() is_running() to the JobHandle maybe that could be
  useful for something

* use highres tiles when they are in cache when zooming out (currently
  only using one level of highres data)


Graphical User Interface
------------------------

* implement auto-pilot that navigates images randomly or in sequence
  (i.e. slideshow with a bit of zooming)

* use framebuffer_object to speed up the drawing

* use graphic indicator for loading (alpha flashing)

* add optional fullscreen-sized cross to indicate cursor position

* add two button mode: hold-right button for drag&drop zoom

* when view is rotated, zooming in position is not rotated with it

* add X11 drag&drop support to SDL

* something useless: add puzzle mode that randomly places tiles on the
  screen and let the user put them back together

* write an auto-fit option: take all pictures fully visible on the
  screen, select them then fit them to the screen

* key stuck issues happens: SDL event queue getting filled up? and key-up events lost?

* add raise/lower buttons

* make 'd' clone the current selection

* add shift-click support to the MoveTool, also rename MoveTool to SelectTool

* add button to flip the view


File Plugins
------------

* handle "Premature end of JPEG file" a little better, mark FileEntry
  as broken or so

* Corel CMX Support

* build in a file magic detector: .xcf.bz2 doesn't work for xcfinfo if it ends in .xcf

* check that all file plugins can load from any URL

* move file-extension -> plugin mapping into configuration file

  -> didn't work out, file format handler have to much interdepedency
     to make this practical right now, could wrap the static plugins
     into boost::function or special plugin objects to make it work

* add support for Imagemagick files that create multiple images (i.e. pdf)

* figure out a way to handle multi-image files (multilayer-xcf)

  XCF: parse xcfinfo output

  Images are accessed when their width/height is requested and when
  the tiles are generated, everything else is happening outside with
  the database not the files

* check for xcf2pnm, xcfinfo and other tool binaries on startup

* catch PNG read errors properly


Database
--------

* Cleanup higher zoom levels of jpg files (takes hours to run):

  delete from tiles where rowid in (
    select tiles.rowid from files,tiles
       where files.fileid = tiles.fileid AND files.url glob "*.jpg" AND tiles.scale = 0
  );

* Cleanup stuff from the database (quite slow):

  Slow:
  delete from files where url glob "file:///tmp/*";
  delete from tiles where fileid not in (select fileid from files);

  Faster(?):
  delete from tiles where fileid in (select fileid from files where url glob "file:///tmp/*");
  delete from files where url glob "file:///tmp/*";

* add:

   galapix.sdl split -d infile.sqlite -p "glob-patern" outfile.sqlite

 manual way to split the database:

    # Create the tables
    PRAGMA auto_vacuum = 1;
    CREATE TABLE files (fileid INTEGER PRIMARY KEY AUTOINCREMENT,url TEXT UNIQUE, size INTEGER, mtime INTEGER, width INTEGER, height INTEGER);
    CREATE UNIQUE INDEX files_index ON files ( url );
    CREATE TABLE tiles (fileid INTEGER, scale INTEGER, x INTEGER, y INTEGER, data BLOB, quality INTEGER, format INTEGER);
    CREATE INDEX tiles_index ON tiles ( fileid, x, y, scale );

    # Attach the orignal
    ATTACH "/home/ingo/.galapix/cache.sqlite" as galapix;

    # Copy the data
    INSERT INTO files select * from galapix.files where url glob "*comics/*";
    INSERT INTO tiles select * from galapix.tiles where fileid in (select fileid from files);

    * Table convert:
      create table if not exists files_new (fileid INTEGER PRIMARY KEY  AUTOINCREMENT, url TEXT UNIQUE, size  INTEGER,  mtime     INTEGER, width INTEGER, height INTEGER);
      insert into files_new (fileid, url, size, mtime, width, height) select fileid, filename, filesize, mtime, width, height from files;
      alter table files rename to files_old
      alter table files_new rename to files;

    * delete tile duplicates via:

    DELETE FROM tiles WHERE rowid IN
    (SELECT b.rowid
     FROM tiles AS a, tiles AS b
     WHERE a.fileid = b.fileid AND
                a.x = b.x      AND
                a.y = b.y      AND
            a.scale = b.scale  AND
            a.rowid < b.rowid);

Then shrink via (do this in a seperate step, since it requires plenty
of diskspace, 2x(!) times what the database uses):

VACUUM;

* add --size option to 'galapix export'

* There seems to be a conflict between libboost-thread1.34 and
  libboost-thread1.35 (thread joinable(), maybe others)

* tile_cache order is wrong

* use boost::bind() objects instead of custum ones in DatabaseThread

* seperate files and images tables so that information about archive
  files can be saved and other stuff

* when a file is given as URL and refers to an archive it does not
  work from command line, since URLs are assumed to refer to images

* always process get requests to the database before store requests

* add command line option to not follow links and/or archive files

* Write a tempfile class that creates a tempfile from a Blob in cases
  where a command line tool can't read from stdin or from /dev/stdin,
  because it might depend on the file extension to figure out a type
  (theoretical, don't think any app needs that at the moment)

* make SpaceNavigator a seperate thread, do not poll events, but wait
  for them, use SDL_PushEvent to push data as user event

* write grid traversal thing so that we can detect the tiles to draw even on scaled or rotated images

* add layout option that makes use of treemaps:
  http://www.cs.umd.edu/hcil/treemap-history/

* make keyboard bindings configurable

* implement some fun recursive layout where images get constantly smaller

* add tool to cut an image (simple rectangle or more complex polygonal shapes)

* PRAGMA temp_store_directory = '/home/ingo/.galapix/tmp/';

* play around with attaching databases to copy stuff around for compability

* is it worth to have a get_size(filename) command? or is it better to
  wait for a proper thumbnail to be there?

  -> get_size() has not much benefit, only real benefit is that you
     can layout before a thumbnail is ready, but placeholder rects are
     not pretty and it only works for a few formats anyway, so drop
     and get the smallest possible thumbnail instead and only place
     stuff on the workspace when it is ready

* Add glob pattern URL support: glob:://file:///resources/flickr/* (what would that be good for?!)

* merging databases causes reencoding of the JPEG data due to
  TileEntry storing SoftwareSurface instead of Blob

* implement: Blob Blob::wrap(void* data, int len);

* mouse behaves incorrectly when view is rotated: zooming in doesn't
  zoom in on mouse_pos properly

* Exec::stdout isn't a Blob, cleanup relationship between Blob,
  std::vector<char> and std::vector<uint8_t*> so that we don't need unneeded copies

* need to filter out directories when generating file list for .rar
  (how? since rar doesn't tell you if its a directiory -> use file extension)

* need a way to handle 'maybe' situations, i.e. impossible to tell
  quickly if a URL exist or not, Size is impossible to tell in a quick
  way often as well

* add something to reconstruct a file from the database (as backup in
  case of file loss) (partly done, see 'export')

* make sure that PNM class can handle all PNM formats that XCF can output

* leaving fullscreen doesn't work properly, glViewport size ends up
  being different from the window size

* when relayout track the currently selected item with the camera, so
  that you can switch between random layout and sorted layout without
  losing track of a picture

* some formats (xcf.bz2, stuff in archives) are very slow when it
  comes to getting width/height, would be faster to not get it and
  only get it when generating tiles

* switch to plain Magick libraries, not the C++ version

* add support for checkboard pattern and stuff in the background

* change Framebuffer/Surface so that we can draw images with alpha

* implemented 2x2 Matrix

* implement image rotation

* When thumbgen is running while galapix view is started:

terminate called after throwing an instance of 'SQLiteError'
  what():  SQLiteStatement::~SQLiteStatement: column filename is not unique
Aborted

* support for additional image formats would be nice, via ImageMagicks
  "convert" as a fallback


Galapix Video Demo
------------------

http://video.google.com/videoplay?docid=7578014083503114514&hl=en

* create a demo video and publish on Google Video
  - demonstrate the direct zoom into thumbnails
  - demonstrate the view rotation
  - demonstrate relayouting of images
  - demonstrate random layout
  - demonstrate the movement of images
  - demonstrate the resize of images
  - demonstrate isolation and deletion of images

* make sure that it isn't as blurry as the last one and doesn't have
  the framerate glitch

* use a wider range of pictures

* use Big Buck Bunny

    // Get a list of all tiles for a given file
    // SELECT files.fileid,files.filename,tiles.scale,tiles.x,tiles.y
    //   FROM files, tiles
    //   WHERE files.filename = 'file:///tmp/ssc2008-11a14.jpg' AND files.fileid = tiles.fileid;
    // When no tiles are available nothing is returned, which makes this call kind of useless for the intended purpose

* added toggle for GL_LINEAR/GL_NEAREST

* seperate loading of tiles from drawing, making loading/job-cancel
  event based instead of brute force as it is now

* generate all tiles at once for stuff that doesn't support
  incremental loading (every other format then jpeg and jpegs loaded
  from archive files) [mostly done, except for jpegs in archive files]

* check the new features some more

* handle file failures more gracefully, currently we run into a 'job
  rejected' situation because the job can never be completed

* Loading JPEG file can throw exception and that isn't handled
  - failure modes: file not found, data corrupt

* maybe limit the scrolling area, so that one can't scroll completly
  out of the picture: calculate the center of all images and let an
  arrow point into the right direction when no images are visible on
  the screen

* improve overlap constrain solver, make it fast and make it animated

* images are always requested in the highest quality, instead lower
  quality should be requested first and heigher quality only if the
  image is visible for longer periodes of time on the screen

* SDL returns different mouse button numbers depending in if a grab is
  active or not, so buttons will get swapped in left-hand situations

* add more command line options:

  --aspect
  --select (?)

* save color or a 8x8 thumbnail in the FileEntry, for faster loading,
  also save 1x1 Color in FileEntry

* store the max_tile size into the FileTable (should we? We can calc
  it dynamically)

* opening a large directory causes file entries to be generated, but
  no tiles to be shown, because tiles are last in the queue, different
  jobs should have different queue

* Displaying 75'000 images at once gets really slow, OpenGL
  bottleneck, texture packing might help, sending vertex array instead
  of glBegin/glEnd of course too (unlikely that anything of that will
  be enough -> framebuffer should be used to only redraw the needed parts)

* implement smooth scale in SoftwareSurface (like LINEAR instead of NEAREST),

* implement non-copying crop in SoftwareSurface (use parent image data
  as pixel source)

* multi-threading in the tile generator for multicore

* faster incremental tile generation

* File stuff should really be seperate from Tile getting -> Seperate
  worker thread?


Database Stuff
--------------

* There is no handling of tile collisions in the database, so one
  might end up with duplicate entries for the same tile. Tile
  generation is also not optimal, since some data gets discarded.

* Support for multiple databases at once would be good, since a
  database can be used/abused as image file


Thread Stuff
------------

* Jobs are currently not prioritized properly so unimportant ones
  might be executed before important ones, not harmfull, but its
  unpleasing to see images on the edge of the screen being generated
  before images in the center of the screen.

* request lower res tiles, before requesting highres tiles

* move JPEG decoding in a seperate thread so that the database thread
  can run free with database stuff


Possible Future Improvements
----------------------------

* crazy stuff: extend Exec() so that stdout from one Exec() can be
  redirected into another Exec(), could be a bit tricky to get the
  whole archive extraction to connected to the file extraction.

* float isn't enough, on high zoom level it breaks apart, might need double

* implement shader to change hue, saturation, contrast, brightness, etc.

* Toolpalette: Select picture, scale picture, move pictures, align
  selected picture into a group, maybe have a box into which one can
  drop pictures.

* When zooming into an images beyond its original size one will see
  noticable seams at the areas where the tiles meet. This is an
  artifact caused by OpenGLs linear filtering that can't work across
  tile borders, but only on a single tile. A possible fix for this
  would be to add a 1px width border to the tiles, but that would slow
  down the tile generation process from JPEG data, so it is unclear if
  it ever will be implemented.

* galapix completly ignores ~/.thumbnails/, it should maybe make use of
  that to speed up initial thumbnail generation

* Some obscure JPEG formats are not supported:
  Generating tiles for /resources/flickr/flickr/farm3.static.flickr.com/2123/1782745732_d7f5c8cc8c_o.jpg
  Exception: JPEG: Unsupported color depth: 4

* allow rotation of images

* record how long an image has been viewed and allow to display which
  images have been viewed most

* support for greyscale in SoftwareSurface (not really important)


Random Notes
------------

* jpegtran can't do crop of large scale JPEGs in small amount of
  space, so cropping with libjpeg likely won't work or get more
  complicated

* a single 2048x2048 texture can hold 65536 8x8 tiles -> ~12MB

* when 8x8 or smaller JPEG is larger then RAW, else JPEG always wins

8x8   -> 250
16x16 -> 430
32x32 -> 700
64x64 -> 1600

1920x1080: (64x64) 30x17=510, (32x32) 60x34=2040, (16x16) 120x68=8160, (4x4) 480x270=129600
~100MB for fullscreen 16x16 Thumbs
~32 textures for 129600 thumbs

16x16 -> 1KB
1000 thumbnails -> 1MB
10'000 thumbnails -> 10MB
100'000 thumbnails -> 100MB (graphic card gets full) -> Solution: smaller thumbnails, also unload from GFX card, but keep software backstore

## Session handoff (2026-09-07)

### Bundles / tips
* Galapix develop tip: apply **`galapix-048.bundle`** (or later). CLI trimmed; viewer default.
* Thumtoo tip: apply **`thumtoo-022.bundle`** (Location URI API).
* Thumtoo tip: apply **`thumtoo-006.bundle`** (or later). Parallel workers, extract cache, wall vs cpu stats.

### Done this arc (Galapix)
* Thumtoo flake input (`flake = false` source tree); **no** in-tree `patches/thumtoo-*.patch`
* Policy: future thumtoo forks → **git subtree**, upstream in batches ([AGENTS.md](AGENTS.md))
* SDL focus; GTK removed from tree
* Tile miss: request overview + coarser parent; cancel keeps coarser jobs
* `ImageOverview` (stdio files, libjpeg scale via `TileGenerator::load_surface`); archives skip overview
* Console status key **`l`** (stdout); overview counts in `print_state`
* **Dear ImGui** status overlay (**F1**); vendored `external/imgui` v1.91.6
* F11 fullscreen toggle
* **CLI trimming:** no subcommands; removed `thumbgen` / `list` / `cleanup` /
  vestigial `info`/`export` and `ThumbnailGenerator`; desktop MimeTypes

### Depends on thumtoo upstream (already on thumtoo master in bundles)
* `request_tile` single-scale only
* Multi-worker Client pool (`--jobs`)
* Extract cache + archive coalesce for tiles/ladder
* `thumtoo-prepare --tiles --stats` timings

### Next session (Galapix) — priority order
1. `nix flake lock --update-input thumtoo` after pulling latest thumtoo; rebuild SDL viewer
2. UI smoke: zoom, edge tiles, resize, F1 overlay, `l` stats with thumtoo on
3. Expand ImGui chrome (tools/workspace) **without** burying logic in widgets
4. Optional: EXIF thumb for overview; persistent overview disk cache
5. Upstream still open: **single-cell** `request_tile` cut (not whole scale grid)
6. Merge develop → master when UI smoke is acceptable (CLI now closer to a clean default)

### Do not
* Reintroduce `pkgs.applyPatches` for thumtoo
* Put business logic inside ImGui callbacks
* Re-add Galapix thumbgen/list/cleanup CLI (belongs in thumtoo)

## CLI trimming ([x] done, 2026-09-07)

**Goal:** Drop subcommands that belong to thumtoo / are obsolete; make the
viewer the default (no `view` prefix required).

Removed:
* `thumbgen` — thumbnail / limited tile gen → use **thumtoo** / `thumtoo-prepare`
* `list` — list DB files → thumtoo / resource tooling
* `cleanup` — DB GC → thumtoo
* Vestigial `info` / `export` methods and `ThumbnailGenerator` sources

Viewer is the only path: non-option arguments are files/URLs; empty argv
launches an empty workspace when the frontend allows it. Leading `view` is
still accepted for old scripts; other former commands error with a pointer
to thumtoo.

**Touched:**
* `src/galapix/arg_parser.cpp`, `galapix.cpp`, `galapix.hpp`
* Removed `thumbnail_generator.{hpp,cpp}`
* `galapix.desktop` — Exec=`galapix %F`; image MimeTypes
* AGENTS.md, README.md, NEWS.md, docs/THUMTOO.md, docs/DEVELOP_VS_MASTER.md,
  docs/TILE_LOADING.md, flake.nix help text


## PDF page tiles show as purple (2026-09-07)

### Symptom
Opening a PDF expands pages (`file:///doc.pdf//page:N`) correctly. Each page
appears in the workspace as a purple “no image” / loading tile. Tiles never
arrive; overview may also stay empty for PDF pages.

### Root cause (thumtoo, not Galapix UI)
In `thumtoo` `Client` tile path (`src/client.cpp`):

- Interactive `request_tile`: when URI is a PDF page, code does
  `reply_one(std::nullopt); return;`
- Pyramid path: `reply_pyramid_done(false); return;`

Size probe and ladder/pixels paths **do** handle PDF (72 dpi media-box size +
`pdf_rasterize_page` → `build_ladder_rgb`). Grid tiles were left as a stub
(“Video/PDF page tiles” still listed as future work in thumtoo TODO).

Galapix `ThumtooTileProvider` correctly requests tiles; it receives empty
blobs → purple stand-ins.

### Fix plan (belongs in thumtoo)
1. Add `build_tile_cell_rgb` / optional pyramid-from-RGB in `image.hpp`/`image.cpp`
   (mirror `build_tile_cell_buffer` using `vips_image_new_from_memory` for RGB888).
2. In `Client` EnsureTiles / interactive tile job:
   - Parse PDF URI
   - Rasterize page at long-edge matching the stored size (72 dpi) or higher if
     we later raise reported size
   - `build_tile_cell_rgb` (or buffer path) for the requested (scale,x,y)
   - Store tile + reply
3. Same for pyramid prewarm.
4. Optional follow-up: report higher native size (e.g. 150–300 dpi) so scale-0
   tiles are useful for reading; keep content_id stable (`sha256:…:page:N`).

### Galapix side after thumtoo fix
- `nix flake lock --update-input thumtoo` (or temporary path/subtree if needed)
- Rebuild SDL viewer; smoke open multi-page PDF, zoom, `l` / F1 stats
- No Galapix logic change expected if URI + provider already correct

### Status
- [x] Implement PDF tile encode in thumtoo (local: `build_tile_cell_rgb` +
      client EnsureTiles path; commit message “Implement PDF page grid tiles”)
- [ ] Land that commit on thumtoo master, then `nix flake lock --update-input thumtoo`
- [ ] UI smoke PDF pages (zoom, multi-page, F1 / `l`)

Until thumtoo master includes the fix, override for local builds:

```bash
nix build .#galapix --override-input thumtoo path:/path/to/thumtoo-with-pdf-tiles
```

