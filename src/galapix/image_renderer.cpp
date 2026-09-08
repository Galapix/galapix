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

#include "galapix/image_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <surf/color.hpp>
#include <wstdisplay/graphics_context.hpp>

#include "galapix/image.hpp"
#include "galapix/image_tile_cache.hpp"
#include "math/math.hpp"
#include "math/rect.hpp"

namespace galapix {

using namespace surf;

namespace {

/** Semi-transparent fill colour encoding the pyramid scale of the pixels
 *  actually drawn (not merely "exact vs stand-in").
 *  Finer scales (lower / more negative) skew cool; coarser skew warm.
 *  Exact match for the requested scale gets a stronger green bias. */
Color
debug_fill_for_scale(int tile_scale, int requested_scale)
{
  // Stable distinct hues for successive scale indices (wrap).
  // RGB primaries chosen for contrast on both light and dark tiles.
  static constexpr uint8_t kPalette[][3] = {
    {  40, 200,  80 }, // 0 green
    {  60, 180, 220 }, // 1 cyan
    {  80, 120, 255 }, // 2 blue
    { 160, 100, 255 }, // 3 violet
    { 220,  80, 200 }, // 4 magenta
    { 240,  90,  90 }, // 5 red
    { 240, 150,  50 }, // 6 orange
    { 230, 210,  40 }, // 7 yellow
    { 160, 220,  50 }, // 8 lime
    {  50, 210, 160 }, // 9 teal
    { 100, 160, 240 }, // 10 light blue
    { 200, 120, 160 }, // 11 rose
  };
  constexpr int n = static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));
  // Bias so typical raster max_scale (~positive) and scale 0 land on different slots.
  int idx = tile_scale % n;
  if (idx < 0) {
    idx += n;
  }

  uint8_t r = kPalette[idx][0];
  uint8_t g = kPalette[idx][1];
  uint8_t b = kPalette[idx][2];

  // Exact requested scale: pull toward saturated green so "good" cells pop.
  if (tile_scale == requested_scale) {
    r = static_cast<uint8_t>((static_cast<int>(r) + 0) / 2);
    g = static_cast<uint8_t>(std::min(255, static_cast<int>(g) + 40));
    b = static_cast<uint8_t>((static_cast<int>(b) + 0) / 2);
  }

  // ~35% opacity — readable tint without hiding tile content.
  return Color::from_rgba8888(r, g, b, 90);
}

Color
debug_outline_for_scale(int tile_scale, int requested_scale)
{
  Color c = debug_fill_for_scale(tile_scale, requested_scale);
  // Solid outline in the same hue.
  return Color::from_rgba8888(c.r8(), c.g8(), c.b8(), 255);
}

int
downscale_to_steps(int downscale)
{
  int k = 0;
  for (int d = downscale; d > 1; d >>= 1) {
    ++k;
  }
  return k;
}

void
draw_tile_debug_overlay(wstdisplay::GraphicsContext& gc, Rectf const& tile_rect,
                        int displayed_scale, int requested_scale)
{
  gc.fill_rect(tile_rect, debug_fill_for_scale(displayed_scale, requested_scale));
  gc.draw_rect(tile_rect, debug_outline_for_scale(displayed_scale, requested_scale));
}

} // namespace

ImageRenderer::ImageRenderer(Image& image, std::shared_ptr<ImageTileCache> const& cache) :
  m_image(image),
  m_cache(cache)
{
}

Vector2f
ImageRenderer::get_vertex(int x, int y, float zoom) const
{
  float tilesize = 256.0f * zoom;

  return m_image.get_top_left_pos() +
    geom::fsize(std::min(static_cast<float>(x) * tilesize,
                         m_image.get_scaled_width()),
                std::min(static_cast<float>(y) * tilesize,
                         m_image.get_scaled_height()));
}

ImageRenderer::ViewPlan
ImageRenderer::plan_view(Rectf const& cliprect, float zoom)
{
  ViewPlan plan;
  Rectf image_rect = m_image.get_image_rect();

  if (!geom::intersects(cliprect, image_rect)) {
    return plan; // visible = false
  }
  plan.visible = true;

  int const desired_scale = std::clamp(
    static_cast<int>(std::floor(
      std::log(1.0f / (zoom * m_image.get_scale())) / std::log(2.0f) + 1e-5f)),
    m_cache->get_min_scale(), m_cache->get_max_scale());
  plan.tiledb_scale = m_cache->stable_request_scale(desired_scale);
  plan.scale_factor = std::ldexp(1.0f, plan.tiledb_scale);
  plan.tile_zoom = plan.scale_factor * m_image.get_scale();

  float const scaled_width  = static_cast<float>(m_image.get_original_width())  / plan.scale_factor;
  float const scaled_height = static_cast<float>(m_image.get_original_height()) / plan.scale_factor;

  if (scaled_width < 256.0f && scaled_height < 256.0f) {
    plan.one_cell = true;
    auto const& ov = m_image.overview();
    // Gallery / fit-all: skip the one-cell grid only when a *sharp* soft
    // underlay is Ready (levels). LQIP-only must not skip_grid — otherwise
    // grid tiles never load and the blurry ThumbHash stays forever.
    if (ov.state() == ImageOverview::State::Ready && ov.has_surface() &&
        !ov.is_lqip_only()) {
      plan.skip_grid = true;
    } else {
      plan.tile_rect = Rect(0, 0, 1, 1);
    }
  } else {
    Rectf image_region = geom::intersection(image_rect, cliprect);
    image_region = Rectf((image_region.left()   - image_rect.left()) / m_image.get_scale(),
                         (image_region.top()    - image_rect.top())  / m_image.get_scale(),
                         (image_region.right()  - image_rect.left()) / m_image.get_scale(),
                         (image_region.bottom() - image_rect.top())  / m_image.get_scale());

    float const itilesize = 256.0f * plan.scale_factor;
    int start_x = static_cast<int>(std::floor(image_region.left() / itilesize));
    int end_x   = static_cast<int>(std::ceil(image_region.right() / itilesize));
    int start_y = static_cast<int>(std::floor(image_region.top() / itilesize));
    int end_y   = static_cast<int>(std::ceil(image_region.bottom() / itilesize));
    plan.tile_rect = Rect(start_x, start_y, end_x, end_y);
  }

  return plan;
}

void
ImageRenderer::prepare(Rectf const& cliprect, float zoom)
{
  ViewPlan const plan = plan_view(cliprect, zoom);

  if (!plan.visible) {
    m_cache->cleanup();
    return;
  }

  // Soft overview / levels first (may consume request budget on thumtoo path).
  // Request overview sized to on-screen long edge (not a fixed 512).
  int const disp_long = static_cast<int>(std::ceil(
    std::max(m_image.get_scaled_width(), m_image.get_scaled_height())));
  m_image.overview().ensure_requested(
    m_image.job_manager(),
    m_image.get_url(),
    m_image.get_original_width(),
    m_image.get_original_height(),
    m_image.get_tile_provider(),
    disp_long);

  if (plan.skip_grid) {
    return;
  }

  // Wait for the *first* soft pixels before stampeding the grid. Once any
  // underlay exists (LQIP or levels), keep marking tiles even while a levels
  // upgrade is Loading — otherwise grid tiles never start and LQIP sticks.
  {
    auto const& ov = m_image.overview();
    auto const st = ov.state();
    if (!ov.has_surface() &&
        (st == ImageOverview::State::Idle ||
         st == ImageOverview::State::Loading)) {
      return;
    }
  }

  if (plan.one_cell) {
    m_cache->cancel_jobs(Rect(0, 0, 1, 1), plan.tiledb_scale);
    m_cache->mark_tile_needed(0, 0, plan.tiledb_scale);
    return;
  }

  m_cache->cancel_jobs(plan.tile_rect, plan.tiledb_scale);
  for (int y = plan.tile_rect.top(); y < plan.tile_rect.bottom(); ++y) {
    for (int x = plan.tile_rect.left(); x < plan.tile_rect.right(); ++x) {
      m_cache->mark_tile_needed(x, y, plan.tiledb_scale);
    }
  }
}

void
ImageRenderer::draw_tile(wstdisplay::GraphicsContext& gc, int x, int y, int scale, float zoom)
{
  // Pure lookup — requests were issued in prepare / issue_requests.
  ImageTileCache::SurfaceStruct sstruct = m_cache->lookup_tile(x, y, scale);
  Rectf const tile_rect(get_vertex(x,   y,   zoom),
                        get_vertex(x+1, y+1, zoom));
  if (sstruct.surface)
  {
    sstruct.surface->draw(gc, tile_rect);

    if (ImageTileCache::tile_debug())
    {
      // Semi-transparent scale tint + outline (exact requested scale).
      draw_tile_debug_overlay(gc, tile_rect, scale, scale);
    }
    return;
  }

  // Exact tile missing: paint coarser stand-in first (tiles that were on
  // screen before zoom-in), then any finer children that already arrived.
  int downscale = 1;
  wstdisplay::SurfacePtr stand_in = m_cache->find_smaller_tile(x, y, scale, downscale);

  if (stand_in)
  {
    // Subsection of the coarser tile that covers this fine cell.
    // downscale == 2^k; pixel size of this cell inside the coarser 256² tile.
    int const sub = std::max(1, 256 / downscale);
    float const sx = static_cast<float>((x % downscale) * sub);
    float const sy = static_cast<float>((y % downscale) * sub);
    Rectf subsection(Vector2f(sx, sy), Sizef(Size(sub, sub)));

    subsection = geom::frect(subsection.topleft(),
                             geom::fpoint(std::min(subsection.right(),  stand_in->get_width()),
                                          std::min(subsection.bottom(), stand_in->get_height())));

    stand_in->draw(gc, subsection, tile_rect);
    if (ImageTileCache::tile_debug()) {
      // Upscaled coarser pyramid level — tint encodes that level's scale.
      int const displayed_scale = scale + downscale_to_steps(downscale);
      draw_tile_debug_overlay(gc, tile_rect, displayed_scale, scale);
    }
  }
  else if (sstruct.status == ImageTileCache::SurfaceStruct::SURFACE_REQUESTED)
  {
    // Prefer soft underlay (LQIP / levels). Opaque purple only when there is
    // truly nothing under the cell yet (overview Idle/Failed, no surface).
    auto const& ov = m_image.overview();
    if (ov.has_surface()) {
      // Soft underlay already drawn — light tint only in tile-debug? skip fill.
      if (ImageTileCache::tile_debug()) {
        gc.fill_rect(tile_rect, Color::from_rgba8888(155, 0, 155, 70));
        gc.draw_rect(tile_rect, Color::from_rgb888(155, 0, 155));
      }
    } else if (ov.state() == ImageOverview::State::Loading) {
      // LQIP/levels in flight — do not cover with opaque purple.
      if (ImageTileCache::tile_debug()) {
        gc.draw_rect(tile_rect, Color::from_rgb888(155, 0, 155));
      }
    } else {
      gc.fill_rect(tile_rect, Color::from_rgb888(155, 0, 155));
    }
  }

  // Progressive refine: draw any finer children that are already loaded.
  wstdisplay::SurfacePtr nw = m_cache->get_tile(2*x,   2*y,   scale - 1);
  wstdisplay::SurfacePtr ne = m_cache->get_tile(2*x+1, 2*y,   scale - 1);
  wstdisplay::SurfacePtr sw = m_cache->get_tile(2*x,   2*y+1, scale - 1);
  wstdisplay::SurfacePtr se = m_cache->get_tile(2*x+1, 2*y+1, scale - 1);
  if (nw) { nw->draw(gc, Rectf(get_vertex(2*x+0, 2*y+0, zoom/2.0f), get_vertex(2*x+1, 2*y+1, zoom/2.0f))); }
  if (ne) { ne->draw(gc, Rectf(get_vertex(2*x+1, 2*y+0, zoom/2.0f), get_vertex(2*x+2, 2*y+1, zoom/2.0f))); }
  if (sw) { sw->draw(gc, Rectf(get_vertex(2*x+0, 2*y+1, zoom/2.0f), get_vertex(2*x+1, 2*y+2, zoom/2.0f))); }
  if (se) { se->draw(gc, Rectf(get_vertex(2*x+1, 2*y+1, zoom/2.0f), get_vertex(2*x+2, 2*y+2, zoom/2.0f))); }
}

void
ImageRenderer::draw_tiles(wstdisplay::GraphicsContext& gc, Rect const& rect, int scale, float zoom)
{
  for(int y = rect.top(); y < rect.bottom(); ++y) {
    for(int x = rect.left(); x < rect.right(); ++x) {
      draw_tile(gc, x, y, scale, zoom);
    }
  }
}

bool
ImageRenderer::draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom)
{
  ViewPlan const plan = plan_view(cliprect, zoom);

  if (!plan.visible) {
    return false;
  }

  Rectf image_rect = m_image.get_image_rect();
  // Soft whole-image preview under tiles (load was requested in prepare).
  m_image.overview().draw(gc, image_rect);

  // Purple only when Idle with nothing queued — Loading often means LQIP is
  // already in the upload queue (process next frame) or levels are in flight.
  if (!m_image.overview().has_surface() &&
      m_image.overview().state() == ImageOverview::State::Idle) {
    gc.fill_rect(image_rect, surf::Color::from_rgb888(155, 0, 155));
  }

  if (plan.skip_grid) {
    return true;
  }

  if (plan.one_cell) {
    draw_tile(gc, 0, 0, plan.tiledb_scale, plan.tile_zoom);
  } else {
    draw_tiles(gc, plan.tile_rect, plan.tiledb_scale, plan.tile_zoom);
  }

  return true;
}

} // namespace galapix

/* EOF */
