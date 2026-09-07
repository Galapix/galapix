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
#include "server/database_thread.hpp"
#include "util/weak_functor.hpp"

namespace galapix {

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
  m_min_keep_scale(m_max_scale - 2)
{
}

wstdisplay::SurfacePtr
ImageTileCache::get_tile(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < 0)
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

ImageTileCache::SurfaceStruct
ImageTileCache::request_tile(int x, int y, int scale)
{
  TileCacheId cache_id(Vector2i(x, y), scale);

  Cache::iterator i = m_cache.find(cache_id);

  // Re-request when a previous attempt failed (e.g. scale outside provider
  // range before max_scale was fixed, or transient decode error).
  bool const need_request =
    (i == m_cache.end()) ||
    (i->second.status == SurfaceStruct::SURFACE_REQUESTED &&
     i->second.job_handle.is_failed() &&
     !i->second.surface);

  if (need_request)
  {
    JobHandle job_handle = m_tile_provider->request_tile(
      scale, Vector2i(x, y),
      weak(std::mem_fn(&ImageTileCache::receive_tile), shared_from_this()));

    // FIXME: Something to try: Request the next smaller tile too,
    // so we get a lower quality image fast and a higher quality one
    // soon after FIXME: Its unclear if this actually improves
    // things, also the order of request gets mungled in the
    // DatabaseThread, we should request the whole group of lower
    // res tiles at once, instead of one by one, since that eats up
    // the possible speed up

    SurfaceStruct surface_struct(job_handle,
                                 SurfaceStruct::SURFACE_REQUESTED,
                                 wstdisplay::SurfacePtr());
    m_cache[cache_id] = surface_struct;
    return surface_struct;
  }
  else
  {
    return i->second;
  }
}

void
ImageTileCache::clear()
{
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    i->second.job_handle.set_aborted();
  }
  m_cache.clear();
}

void
ImageTileCache::cleanup()
{
  // Cancel all jobs and remove tiles smaller m_min_keep_scale
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end();)
  {
    if (i->second.status == SurfaceStruct::SURFACE_REQUESTED)
    {
      i->second.job_handle.set_aborted();
      m_cache.erase(i++);
    }
    else if (i->second.status == SurfaceStruct::SURFACE_SUCCEEDED &&
             i->first.get_scale() < m_min_keep_scale)
    {
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
  int  downscale_factor = 1;

  while(downscale_factor < m_max_scale)
  {
    downscale_out = Math::pow2(downscale_factor);

    TileCacheId cache_id(Vector2i(x / downscale_out, y / downscale_out),
                         tiledb_scale+downscale_factor);

    Cache::iterator i = m_cache.find(cache_id);
    if (i != m_cache.end() && i->second.surface)
    {
      return i->second.surface;
    }

    downscale_factor += 1;
  }

  return {};
}

void
ImageTileCache::process_queue()
{
  // Cap GL uploads per frame so fast zoom never stalls the UI. Remaining
  // tiles stay queued; receive_tile / this function request another redraw.
  constexpr int kMaxUploadsPerFrame = 4;

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

  if (uploaded > 0 && !m_tile_queue.empty())
  {
    // More decoded tiles waiting — keep animating without blocking this frame.
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
  if (!m_cache.empty())
  {
    for(Cache::iterator i = m_cache.begin(); i != m_cache.end();)
    {
      if (i->second.status == SurfaceStruct::SURFACE_REQUESTED &&
          (scale != i->first.get_scale() || !geom::contains(rect, i->first.get_pos())))
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
    if (entry.second.status == SurfaceStruct::SURFACE_REQUESTED) {
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

} // namespace galapix

/* EOF */
