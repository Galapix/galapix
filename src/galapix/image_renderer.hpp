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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_RENDERER_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_RENDERER_HPP

#include <memory>

#include <wstdisplay/fwd.hpp>

#include "math/vector2f.hpp"
#include "math/rect.hpp"

namespace galapix {

class Image;
class ImageTileCache;

class ImageRenderer
{
public:
  ImageRenderer(Image& image, std::shared_ptr<ImageTileCache> const& cache);

  /** Camera-relative draw origin (world space, double). Geometry is emitted as
      float(local = world - origin) so the float modelview stays well-conditioned.
      OpenGL matrices remain float32 (wstdisplay / GPU); this is the stable path. */
  static void set_render_origin(Vector2d const& origin);
  static void clear_render_origin();
  static Vector2d render_origin() { return s_render_origin; }

  /** Visibility, cancel obsolete jobs, mark needed tiles, request overview.
      No GL draws. Call once per frame before issue_requests / draw. */
  void prepare(Rectf const& cliprect, float zoom);

  /** Pure draw: overview + tile surfaces / stand-ins / placeholders.
      Must not enqueue provider work. */
  bool draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom);

  /** World → draw space (subtract render origin, then float). */
  static Vector2f to_draw(Vector2f const& world);
  static Rectf to_draw(Rectf const& world);

private:
  Vector2f get_vertex(int x, int y, float zoom) const;
  void draw_tile(wstdisplay::GraphicsContext& gc, int x, int y, int tiledb_scale, float zoom);
  void draw_tiles(wstdisplay::GraphicsContext& gc, Rect const& rect, int tiledb_scale, float zoom);

  /** Shared visibility / LOD decision for prepare and draw. */
  struct ViewPlan
  {
    bool visible = false;
    bool one_cell = false;
    bool skip_grid = false; // gallery: overview covers us
    int tiledb_scale = 0;
    float scale_factor = 1.0f;
    float tile_zoom = 1.0f;
    Rect tile_rect{0, 0, 0, 0}; // grid cells when !one_cell
  };

  ViewPlan plan_view(Rectf const& cliprect, float zoom);

private:
  Image& m_image;
  std::shared_ptr<ImageTileCache> m_cache;
  static Vector2d s_render_origin;

private:
  ImageRenderer(ImageRenderer const&);
  ImageRenderer& operator=(ImageRenderer const&);
};

} // namespace galapix

#endif

/* EOF */
