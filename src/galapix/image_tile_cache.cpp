// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "galapix/image_tile_cache.hpp"

#ifdef HAVE_THUMTOO
#  include "thumtoo/thumtoo_tile_provider.hpp"
#endif

#include <algorithm>
#include <vector>

#include <assert.h>
#include <cstring>

#include <glad/gl.h>
#include <surf/blit.hpp>
#include <surf/fill.hpp>
#include <surf/pixel.hpp>
#include <surf/software_surface.hpp>
#include <wstdisplay/texture.hpp>

#include "galapix/viewer.hpp"
#include "math/math.hpp"
#include "util/weak_functor.hpp"

namespace galapix {


bool ImageTileCache::s_tile_debug = false;
bool ImageTileCache::s_tile_requests_enabled = true;

namespace {
int s_request_budget = 0;
int s_frame_uploads = 0;
int s_frame_requests_started = 0;
} // namespace

void
ImageTileCache::begin_frame_request_budget(int max_new_requests)
{
  s_request_budget = max_new_requests;
  s_frame_uploads = 0;
  s_frame_requests_started = 0;
}

bool
ImageTileCache::try_consume_request_budget()
{
  if (s_request_budget <= 0) {
    return false;
  }
  --s_request_budget;
  ++s_frame_requests_started;
  return true;
}

int
ImageTileCache::frame_uploads()
{
  return s_frame_uploads;
}

int
ImageTileCache::frame_requests_started()
{
  return s_frame_requests_started;
}

int
ImageTileCache::request_budget_remaining()
{
  return s_request_budget;
}


namespace {

/** Pad a partial edge tile to tile_size² by repeating the last row/column
 *  (edge-clamp). LINEAR filtering near UV boundaries then samples content
 *  instead of undefined / black texels — fixes the black bottom row on
 *  right-edge tiles.
 */
surf::SoftwareSurface
pad_tile_edge_clamp(surf::SoftwareSurface const& src, int tile_size)
{
  int const w = src.get_width();
  int const h = src.get_height();
  if (w <= 0 || h <= 0) {
    return src;
  }
  if (w >= tile_size && h >= tile_size) {
    return src;
  }

  auto dst = surf::SoftwareSurface::create(
    surf::PixelFormat::RGBA8, geom::isize(tile_size, tile_size));
  // Source may be RGB8 or RGBA8; blit converts as needed.
  surf::blit(src, dst, geom::ipoint(0, 0));

  auto& view = dst.as_pixelview<surf::RGBA8Pixel>();

  // Extend right: copy last valid column.
  if (w < tile_size) {
    for (int y = 0; y < h; ++y) {
      surf::RGBA8Pixel* row = view.get_row(y);
      for (int x = w; x < tile_size; ++x) {
        row[x] = row[w - 1];
      }
    }
  }

  // Extend bottom: copy last valid row (already right-extended if needed).
  if (h < tile_size) {
    surf::RGBA8Pixel const* src_row = view.get_row(h - 1);
    for (int y = h; y < tile_size; ++y) {
      std::memcpy(view.get_row(y), src_row,
                  static_cast<size_t>(tile_size) * sizeof(surf::RGBA8Pixel));
    }
  }

  return dst;
}

/** Upload tile without mipmaps; pad edge tiles so LINEAR never hits black. */
wstdisplay::SurfacePtr
surface_from_software(surf::SoftwareSurface const& image)
{
  constexpr int kTile = 256;
  int const content_w = image.get_width();
  int const content_h = image.get_height();

  surf::SoftwareSurface upload = image;
  if (content_w < kTile || content_h < kTile) {
    upload = pad_tile_edge_clamp(image, kTile);
  }

  // Texture::create(SoftwareSurface) uses gluBuild2DMipmaps; edge tiles have
  // shown a black bottom row with that path. Allocate empty + put level 0 only.
  auto texture = wstdisplay::Texture::create(
    GL_TEXTURE_2D, upload.get_size());
  texture->put(upload, 0, 0);
  texture->set_filter(GL_LINEAR);

  float const tw = static_cast<float>(texture->get_width());
  float const th = static_cast<float>(texture->get_height());
  // UV covers only the real content; padding is filter apron.
  float const maxu = static_cast<float>(content_w) / tw;
  float const maxv = static_cast<float>(content_h) / th;

  return wstdisplay::Surface::create(
    texture,
    geom::frect(0.0f, 0.0f, maxu, maxv),
    geom::fsize(static_cast<float>(content_w),
                static_cast<float>(content_h)));
}

} // namespace

ImageTileCache::ImageTileCache(TileProviderPtr const& tile_provider) :
  m_cache(),
  m_tile_queue(),
  m_tile_provider(tile_provider),
  m_max_scale(m_tile_provider->get_max_scale()),
  m_min_scale(m_tile_provider->get_min_scale()),
  // Keep all scales that successfully loaded. Previously max_scale-2 dropped
  // almost everything on pan-off (cleanup), so only ~overview tiles survived
  // (~1k entries for large galleries).
  m_min_keep_scale(m_min_scale),
  m_have_last_cancel(false),
  m_last_cancel_scale(0),
  m_last_cancel_rect(),
  m_have_stable_scale(false),
  m_stable_scale(0),
  m_pending_scale(0),
  m_pending_since()
{
}

void
ImageTileCache::prefetch_overview()
{
  // Prefetch full-image overview so find_smaller_tile has a stand-in before
  // the user zooms into fine scales (avoids purple "loading" cells).
  // Requires a live shared_ptr (uses shared_from_this via queue_tile_request).
  if (m_max_scale >= 0) {
    queue_tile_request(0, 0, m_max_scale);
  }
}

wstdisplay::SurfacePtr
ImageTileCache::get_tile(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < m_min_scale)
  {
    return {};
  }
  else
  {
    TileCacheId cache_id(Vector2i(x, y), scale);
    Cache::iterator i = m_cache.find(cache_id);

    if (i != m_cache.end())
    {
      return i->second.surface;
    }
    else
    {
      return {};
    }
  }
}

void
ImageTileCache::queue_tile_request(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < m_min_scale || scale > m_max_scale) {
    return;
  }

  // Cache-only / debug mode: do not start new provider work.
  if (!s_tile_requests_enabled) {
    return;
  }

  TileCacheId cache_id(Vector2i(x, y), scale);
  Cache::iterator i = m_cache.find(cache_id);

  constexpr int kMaxTileAttempts = 3;
  int next_attempts = 1;
  if (i != m_cache.end()) {
    // Retry failed / aborted cells a few times (live PDF region render is
    // slow and often aborted by cancel_jobs while the user pans/zooms).
    // Without treating is_aborted() like a failure, the entry stays
    // SURFACE_REQUESTED with no surface and is never re-queued → permanent
    // purple cells for live-only / negative-scale PDF tiles.
    bool const dead =
      i->second.status == SurfaceStruct::SURFACE_REQUESTED &&
      !i->second.surface &&
      (i->second.job_handle.is_failed() || i->second.job_handle.is_aborted());
    if (dead && i->second.attempts < kMaxTileAttempts) {
      next_attempts = i->second.attempts + 1;
      m_cache.erase(i);
    } else {
      // Already SUCCEEDED, in-flight, or retries exhausted — do NOT consume
      // the per-frame budget. Exhausted dead cells stay until cancel_jobs.
      return;
    }
  }

  // Global per-frame start budget (begin_frame_request_budget). Only charge
  // when we are about to start a real provider job.
  if (!try_consume_request_budget()) {
    return;
  }

  JobHandle job_handle = m_tile_provider->request_tile(
    scale, Vector2i(x, y),
    weak(std::mem_fn(&ImageTileCache::receive_tile), shared_from_this()));

  m_cache[cache_id] = SurfaceStruct(job_handle,
                                    SurfaceStruct::SURFACE_REQUESTED,
                                    wstdisplay::SurfacePtr(),
                                    next_attempts);
}

void
ImageTileCache::clear_needed()
{
  m_needed.clear();
}

void
ImageTileCache::mark_tile_needed(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < m_min_scale || scale > m_max_scale) {
    return;
  }

  // Only the exact view-scale cell. Coarser ancestors used to be marked for
  // "stand-in" generation; that left dozens of REQUESTED parent jobs that
  // cancel_jobs deliberately kept (scale > view), so pending_requests never
  // drained and tile-debug showed mixed resolutions forever.
  // Display still uses find_smaller_tile() on *already SUCCEEDED* coarser
  // surfaces — no need to request them again.
  m_needed.insert(TileCacheId(Vector2i(x, y), scale));
}

void
ImageTileCache::set_request_focus(float tile_x, float tile_y)
{
  m_focus_tx = tile_x;
  m_focus_ty = tile_y;
}

void
ImageTileCache::issue_requests()
{
  if (m_needed.empty()) {
    return;
  }

  // Finer scales first (exact view cells before parents/overview). Within a
  // scale, nearer the viewport centre first.
  float const fx = m_focus_tx;
  float const fy = m_focus_ty;
  std::vector<TileCacheId> ordered(m_needed.begin(), m_needed.end());
  std::sort(ordered.begin(), ordered.end(),
            [fx, fy](TileCacheId const& a, TileCacheId const& b) {
              if (a.get_scale() != b.get_scale()) {
                return a.get_scale() < b.get_scale(); // finer first
              }
              float const da =
                (static_cast<float>(a.get_pos().x()) + 0.5f - fx) *
                  (static_cast<float>(a.get_pos().x()) + 0.5f - fx) +
                (static_cast<float>(a.get_pos().y()) + 0.5f - fy) *
                  (static_cast<float>(a.get_pos().y()) + 0.5f - fy);
              float const db =
                (static_cast<float>(b.get_pos().x()) + 0.5f - fx) *
                  (static_cast<float>(b.get_pos().x()) + 0.5f - fx) +
                (static_cast<float>(b.get_pos().y()) + 0.5f - fy) *
                  (static_cast<float>(b.get_pos().y()) + 0.5f - fy);
              if (da != db) {
                return da < db;
              }
              if (a.get_pos().x() != b.get_pos().x()) {
                return a.get_pos().x() < b.get_pos().x();
              }
              return a.get_pos().y() < b.get_pos().y();
            });

  // Build a batch of new provider jobs. ThumtooTileProvider coalesces these
  // into one Client job so one decode/ladder serves the whole visible set.
  std::vector<TileProvider::TileRequest> batch;
  batch.reserve(ordered.size());
  constexpr int kMaxTileAttempts = 3;

  for (TileCacheId const& id : ordered) {
    int const x = id.get_pos().x();
    int const y = id.get_pos().y();
    int const scale = id.get_scale();
    if (x < 0 || y < 0 || scale < m_min_scale || scale > m_max_scale) {
      continue;
    }
    if (!s_tile_requests_enabled) {
      break;
    }

    Cache::iterator i = m_cache.find(id);
    int next_attempts = 1;
    if (i != m_cache.end()) {
      bool const dead =
        i->second.status == SurfaceStruct::SURFACE_REQUESTED &&
        !i->second.surface &&
        (i->second.job_handle.is_failed() || i->second.job_handle.is_aborted());
      if (dead && i->second.attempts < kMaxTileAttempts) {
        next_attempts = i->second.attempts + 1;
        m_cache.erase(i);
      } else if (dead) {
        // Exhausted retries: keep the dead entry so we do not re-issue every
        // frame (was burning the global request budget with req=0). cancel_jobs
        // drops it when the view scale/rect changes so a later view can retry.
        continue;
      } else {
        continue; // hit or in-flight
      }
    }

    if (!try_consume_request_budget()) {
      break;
    }

    JobHandle job_handle = JobHandle::create();
    m_cache[id] = SurfaceStruct(job_handle,
                                SurfaceStruct::SURFACE_REQUESTED,
                                wstdisplay::SurfacePtr(),
                                next_attempts);

    batch.emplace_back(
      scale,
      Vector2i(x, y),
      job_handle,
      weak(std::mem_fn(&ImageTileCache::receive_tile), shared_from_this()));
  }

  m_needed.clear();

  if (batch.empty()) {
    return;
  }

#ifdef HAVE_THUMTOO
  // Thumtoo: one backend job for the whole set (shared decode ladder).
  if (std::dynamic_pointer_cast<ThumtooTileProvider>(m_tile_provider)) {
    m_tile_provider->request_tiles(std::move(batch));
    return;
  }
#endif

  // Other providers: one request_tile each (their JobHandle replaces ours).
  for (auto& req : batch) {
    TileCacheId id(req.pos, req.scale);
    JobHandle h = m_tile_provider->request_tile(req.scale, req.pos, req.callback);
    Cache::iterator it = m_cache.find(id);
    if (it != m_cache.end()) {
      it->second.job_handle = h;
    }
  }
}

ImageTileCache::SurfaceStruct
ImageTileCache::lookup_tile(int x, int y, int scale)
{
  TileCacheId cache_id(Vector2i(x, y), scale);
  Cache::iterator i = m_cache.find(cache_id);
  if (i != m_cache.end()) {
    return i->second;
  }
  // Not in cache (budget, disabled, or not yet issued): treat as loading so
  // the draw path can show the purple placeholder instead of a blank cell.
  SurfaceStruct missing;
  missing.status = SurfaceStruct::SURFACE_REQUESTED;
  return missing;
}

ImageTileCache::SurfaceStruct
ImageTileCache::request_tile(int x, int y, int scale)
{
  // Legacy one-shot path: mark stand-ins + target, issue immediately, lookup.
  mark_tile_needed(x, y, scale);
  issue_requests();
  return lookup_tile(x, y, scale);
}

void
ImageTileCache::clear()
{
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    i->second.job_handle.set_aborted();
  }
  m_cache.clear();
  m_have_last_cancel = false;
  m_have_stable_scale = false;
}

void
ImageTileCache::cleanup()
{
  // Off-screen / explicit cache_cleanup: abort in-flight requests only.
  // Retain every SUCCEEDED tile (full scale range). Memory grows with browsed
  // content; clear_cache() / Image reset still wipe everything.
  for (Cache::iterator i = m_cache.begin(); i != m_cache.end();)
  {
    if (i->second.status == SurfaceStruct::SURFACE_REQUESTED)
    {
      i->second.job_handle.set_aborted();
      m_cache.erase(i++);
    }
    else
    {
      ++i;
    }
  }
}

wstdisplay::SurfacePtr
ImageTileCache::find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_out)
{
  // Lookup only — do not enqueue from the draw path. Walking every coarser
  // scale used to fire O(max_scale) provider jobs per missing cell per frame
  // when this path still requested; keep it pure.
  //
  // Prefer the nearest coarser cell that already has a GL surface so zoom-in
  // keeps showing the tiles that were on screen a moment earlier.
  int downscale_factor = 1;
  int const max_factor = m_max_scale - tiledb_scale;

  while (downscale_factor <= max_factor && downscale_factor < 32)
  {
    downscale_out = Math::pow2(downscale_factor);

    int const cx = x / downscale_out;
    int const cy = y / downscale_out;
    int const cscale = tiledb_scale + downscale_factor;

    Cache::iterator i = m_cache.find(TileCacheId(Vector2i(cx, cy), cscale));
    if (i != m_cache.end() && i->second.surface)
    {
      return i->second.surface;
    }

    downscale_factor += 1;
  }

  // Whole-image overview cell (0,0) at max_scale may not lie on the ancestor
  // chain for every (x,y) when max_scale mapping used different alignment —
  // still try it as last resort when present.
  if (max_factor > 0) {
    Cache::iterator i = m_cache.find(TileCacheId(Vector2i(0, 0), m_max_scale));
    if (i != m_cache.end() && i->second.surface) {
      downscale_out = Math::pow2(max_factor);
      // Map fine cell into the single overview tile's pixel space.
      // Caller uses x%downscale / y%downscale; for overview that is wrong if
      // the overview is one cell for the whole image. Use absolute mapping:
      // store downscale_out as full grid size at this scale.
      // Keep conventional downscale so subsection math uses x % downscale.
      return i->second.surface;
    }
  }

  return {};
}


int
ImageTileCache::stable_request_scale(int desired_scale)
{
  using clock = std::chrono::steady_clock;
  // Only damp *adjacent* scale steps (continuous wheel). Large jumps
  // (layout, zoom-to-fit, multi-notch wheel) commit immediately so initial
  // gallery fill is not delayed per level. Hold is long enough that a steady
  // zoom does not re-issue a full intermediate grid every ~3 frames.
  constexpr auto kHold = std::chrono::milliseconds(150);

  if (!m_have_stable_scale) {
    m_have_stable_scale = true;
    m_stable_scale = desired_scale;
    m_pending_scale = desired_scale;
    m_pending_since = clock::now();
    return m_stable_scale;
  }

  if (desired_scale == m_stable_scale) {
    m_pending_scale = desired_scale;
    return m_stable_scale;
  }

  int const delta = desired_scale > m_stable_scale
                      ? desired_scale - m_stable_scale
                      : m_stable_scale - desired_scale;
  if (delta > 1) {
    m_stable_scale = desired_scale;
    m_pending_scale = desired_scale;
    m_pending_since = clock::now();
    return m_stable_scale;
  }

  // Adjacent step: hold until desired stays put briefly.
  if (desired_scale != m_pending_scale) {
    m_pending_scale = desired_scale;
    m_pending_since = clock::now();
    return m_stable_scale;
  }

  if (clock::now() - m_pending_since >= kHold) {
    m_stable_scale = desired_scale;
  }
  return m_stable_scale;
}

bool
ImageTileCache::request_scale_holding() const
{
  return m_have_stable_scale && m_pending_scale != m_stable_scale;
}

void
ImageTileCache::process_queue()
{
  // Cap GL uploads per image per frame. Was 2 and starved gallery fill when
  // many overview tiles were already decoded. Higher budget drains the
  // receive queue faster when thumbnails are cache hits.
  constexpr int kMaxUploadsPerFrame = 64;

  int uploaded = 0;
  Tile tile;
  while (uploaded < kMaxUploadsPerFrame && m_tile_queue.try_pop(tile))
  {
    TileCacheId tile_id(tile.get_pos(), tile.get_scale());

    Cache::iterator i = m_cache.find(tile_id);

    if (i == m_cache.end())
    {
      m_cache[tile_id] = SurfaceStruct(JobHandle::create(),
                                       SurfaceStruct::SURFACE_SUCCEEDED,
                                       surface_from_software(tile.get_surface()));
    }
    else
    {
      i->second.surface = surface_from_software(tile.get_surface());
      i->second.status = SurfaceStruct::SURFACE_SUCCEEDED;
    }
    ++uploaded;
  }
  s_frame_uploads += uploaded;

  // Keep the main loop running while decoded tiles await upload; otherwise
  // pending_upload_count can sit non-zero after workers go idle and no
  // further receive_tile redraws arrive.
  if (uploaded > 0 || !m_tile_queue.empty())
  {
    if (Viewer* v = Viewer::current()) {
      v->redraw();
    }
  }
}

struct TileReqestIsAborted
{
  bool operator()(ImageTileCache::SurfaceStruct const& request)
  {
    return request.job_handle.is_aborted();
  }
};

void
ImageTileCache::cancel_jobs(Rect const& rect, int scale)
{
  // scale 0 = full resolution, higher = coarser.
  // Drop obsolete REQUESTED work when the view moves:
  //  - any scale other than the current view scale (including coarser
  //    "stand-in" requests that used to be kept and left pending stuck)
  //  - same scale but outside the visible tile rect
  // SUCCEEDED surfaces at other scales stay for find_smaller_tile().
  //
  // Skip full-map walk when the visible rect/scale did not change.
  if (m_have_last_cancel && m_last_cancel_scale == scale &&
      m_last_cancel_rect == rect) {
    return;
  }
  m_have_last_cancel = true;
  m_last_cancel_scale = scale;
  m_last_cancel_rect = rect;

  if (m_cache.empty()) {
    return;
  }

  for (Cache::iterator i = m_cache.begin(); i != m_cache.end();)
  {
    if (i->second.status != SurfaceStruct::SURFACE_REQUESTED) {
      ++i;
      continue;
    }

    int const job_scale = i->first.get_scale();
    bool const wrong_scale = job_scale != scale;
    bool const same_scale_outside =
      job_scale == scale && !geom::contains(rect, i->first.get_pos());

    if (wrong_scale || same_scale_outside)
    {
      i->second.job_handle.set_aborted();
      m_cache.erase(i++);
    }
    else
    {
      ++i;
    }
  }
}

void
ImageTileCache::receive_tile(Tile const& tile)
{
  // May run on a Client worker thread: queue is thread-safe; only schedule
  // a redraw (SDL_PushEvent). OpenGL upload happens in process_queue on main.
  m_tile_queue.wait_and_push(tile);

  if (Viewer* v = Viewer::current()) {
    v->redraw();
  }
}

int
ImageTileCache::pending_request_count() const
{
  int n = 0;
  for (auto const& entry : m_cache) {
    if (entry.second.status != SurfaceStruct::SURFACE_REQUESTED) {
      continue;
    }
    // Failed/aborted are not in-flight. Finished means the worker already
    // delivered into m_tile_queue (awaiting GL upload) — that is
    // pending_uploads, not pending_requests. Fresh generate could leave
    // dozens of finished+REQUESTED cells and look "stuck" at ~100.
    if (entry.second.job_handle.is_failed() ||
        entry.second.job_handle.is_aborted() ||
        entry.second.job_handle.is_finished()) {
      continue;
    }
    ++n;
  }
  return n;
}

int
ImageTileCache::ready_surface_count() const
{
  int n = 0;
  for (auto const& entry : m_cache) {
    if (entry.second.status == SurfaceStruct::SURFACE_SUCCEEDED &&
        entry.second.surface) {
      ++n;
    }
  }
  return n;
}

int
ImageTileCache::pending_upload_count() const
{
  return m_tile_queue.size();
}


// Timed reclaim of live REQUESTED was removed. Providers must always
// finish/fail/abort every issued JobHandle. Interactive tile scheduling is
// FIFO in thumtoo so older cells are not starved behind continuous new work.

void
ImageTileCache::dump_stuck_requests(int limit) const
{
  using clock = std::chrono::steady_clock;
  auto const now = clock::now();
  int n = 0;
  for (auto const& entry : m_cache)
  {
    if (entry.second.status != SurfaceStruct::SURFACE_REQUESTED ||
        entry.second.surface)
    {
      continue;
    }
    auto const& h = entry.second.job_handle;
    if (h.is_finished() || h.is_failed() || h.is_aborted())
    {
      continue;
    }
    if (n >= limit)
    {
      std::cout << "  ... more stuck requests omitted\n";
      break;
    }
    double age = std::chrono::duration<double>(now - entry.second.issued_at).count();
    std::cout << "  stuck scale=" << entry.first.get_scale()
              << " pos=(" << entry.first.get_pos().x() << ","
              << entry.first.get_pos().y() << ")"
              << " age=" << age << "s"
              << " attempts=" << entry.second.attempts
              << " finished=" << h.is_finished()
              << " failed=" << h.is_failed()
              << " aborted=" << h.is_aborted()
              << "\n";
    ++n;
  }
  if (n == 0)
  {
    std::cout << "  (no live REQUESTED handles)\n";
  }
}

} // namespace galapix

/* EOF */
