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

#ifndef HEADER_GALAPIX_GALAPIX_VIEWER_STATE_HPP
#define HEADER_GALAPIX_GALAPIX_VIEWER_STATE_HPP

#include "math/vector2f.hpp"
#include "math/rect.hpp"

namespace galapix {

class Viewer;

/** Pan/zoom/rotate of the framebuffer camera.
 *
 *  Offset and scale are **double** so deep zoom (e.g. builtin://mandelbrot)
 *  does not fall apart from float mantissa limits. OpenGL still receives
 *  single-precision matrices at the draw boundary.
 */
class ViewerState
{
public:
  ViewerState(Viewer& viewer);

  /** Zoom the view, keeping pos on the same position of the framebuffer.
      @param zoom  amount by which should be zoomed
      @param pos   position in framebuffer coordinates */
  void zoom(double factor, Vector2i const& pos);
  void zoom(double factor);

  /** pos is in screen coordinates */
  void move(Vector2d const& pos);
  /** Convenience for tools that still pass float deltas. */
  void move(Vector2f const& pos);

  void rotate(double r);
  void set_angle(double r);
  void set_offset(Vector2d const& o);
  void set_offset(Vector2f const& o);
  void set_scale(double s);

  void zoom_to(Size const& display, Rectf const& rect);
  void zoom_to(Size const& display, Rectd const& rect);

  Vector2d screen2world(Vector2i const& pos) const;
  Rectd    screen2world(Rect const& rect) const;
  Vector2f screen2world_f(Vector2i const& pos) const;
  Rectf    screen2world_f(Rect const& rect) const;

  Vector2d get_offset() const { return offset; }
  double   get_scale()  const { return scale; }
  double   get_angle() const  { return angle; }

  /** Float snapshots for APIs that still take float (draw/tools). */
  Vector2f get_offset_f() const {
    return Vector2f(static_cast<float>(offset.x()), static_cast<float>(offset.y()));
  }
  float get_scale_f() const { return static_cast<float>(scale); }
  float get_angle_f() const { return static_cast<float>(angle); }

private:
  Viewer& m_viewer;

  /** scale 1.0 is neutral */
  double scale;

  /** angle in degree, not rad */
  double angle;

  /** top right of the view in scaled inverse world co (-offset/scale → WO) */
  Vector2d offset;
};

} // namespace galapix

#endif

/* EOF */
