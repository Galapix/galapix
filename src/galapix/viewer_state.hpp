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

class ViewerState
{
public:
  ViewerState(Viewer& viewer);

  /** Zoom the view, keeping pos on the same position of the framebuffer.

      @param zoom  amount by which should be zoomed
      @param pos   position in framebuffer coordinates */
  void zoom(float factor, Vector2i const& pos);
  void zoom(float factor);

  /** pos is in screen coordinates */
  void move(Vector2f const& pos);

  void rotate(float r);
  void set_angle(float r);
  void set_offset(Vector2f const& o);
  void set_scale(float s);

  void zoom_to(Size const& display, Rectf const& rect);

  Vector2f screen2world(Vector2i const& pos) const;
  Rectf    screen2world(Rect const& rect) const;

  Vector2f get_offset() const { return offset; }
  float    get_scale()  const { return scale; }
  float    get_angle() const  { return angle; }

private:
  Viewer& m_viewer;

  /** scale 1.0f is neutral */
  float    scale;

  /** angle in degree, not rad */
  float    angle;

  /** top right of the view in scaled inverse world co! (-offset/scale -> WO) */
  Vector2f offset;
};

} // namespace galapix

#endif

/* EOF */
