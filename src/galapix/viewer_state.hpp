/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HEADER_GALAPIX_GALAPIX_VIEWER_STATE_HPP
#define HEADER_GALAPIX_GALAPIX_VIEWER_STATE_HPP

#include "math/vector2f.hpp"
#include "math/rect.hpp"

class ViewerState
{
private:
  /** scale 1.0f is neutral */
  float    scale;

  /** angle in degree, not rad */
  float    angle;

  /** top right of the view in scaled inverse world co! (-offset/scale -> WO) */
  Vector2f offset;
  
public:
  ViewerState();

  void zoom(float factor, const Vector2i& pos);
  void zoom(float factor);
  
  /** pos is in screen coordinates */
  void move(const Vector2f& pos);

  void rotate(float r);
  void set_angle(float r);
  void set_offset(const Vector2f& o);
  void set_scale(float s);

  void zoom_to(const Size& display, const Rectf& rect);

  Vector2f screen2world(const Vector2i&) const;
  Rectf    screen2world(const Rect&) const;

  Vector2f get_offset() const { return offset; }
  float    get_scale()  const { return scale; }
  float    get_angle() const  { return angle; }
};

#endif

/* EOF */
