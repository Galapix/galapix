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

#include "framebuffer.hpp"
#include "viewer_state.hpp"

ViewerState::ViewerState()
  : scale(1.0f),
    angle(0.0f),
    offset(0.0f, 0.0f)
{
}

void
ViewerState::zoom(float factor, const Vector2i& pos)
{
  scale *= factor;

  offset = Vector2f(pos) - ((Vector2f(pos) - offset) * factor);
}

void
ViewerState::zoom(float factor)
{
  zoom(factor, Vector2i(Framebuffer::get_width()/2, // FIXME: Little ugly, isn't it?
                        Framebuffer::get_height()/2));
}

void
ViewerState::rotate(float r)
{
  angle += r;
}

void
ViewerState::set_angle(float r)
{
  angle = r;
}

void
ViewerState::set_offset(const Vector2f& o)
{
  offset = o;
}

void
ViewerState::set_scale(float s)
{
  scale = s;
}  

void
ViewerState::move(const Vector2f& pos)
{
  // FIXME: Implement a proper 2D Matrix instead of this hackery
  offset.x += pos.x * cosf(angle/180.0f*M_PI) +  pos.y * sinf(angle/180.0f*M_PI);
  offset.y -= pos.x * sinf(angle/180.0f*M_PI) -  pos.y * cosf(angle/180.0f*M_PI);
}

Vector2f
ViewerState::screen2world(const Vector2i& pos) const
{
  return (Vector2f(pos) - offset) / scale;
}

Rectf
ViewerState::screen2world(const Rect& rect) const
{
  return Rectf((rect.left   - offset.x) / scale,
               (rect.top    - offset.y) / scale,
               (rect.right  - offset.x) / scale,
               (rect.bottom - offset.y) / scale);
}

/* EOF */
