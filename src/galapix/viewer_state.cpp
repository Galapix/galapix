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

#include "galapix/viewer_state.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>

#include "display/framebuffer.hpp"

ViewerState::ViewerState() :
  scale{1.0f},
  angle{0.0f},
  offset{0.0f, 0.0f}
{
}

void
ViewerState::zoom(float factor, const Vector2i& pos)
{
  scale *= factor;

  Vector2f center(static_cast<float>(Framebuffer::get_width())  / 2.0f,
                  static_cast<float>(Framebuffer::get_height()) / 2.0f);

  Vector2f rotated_pos(pos);
  rotated_pos -= center;
  rotated_pos = glm::rotate(rotated_pos, glm::radians(-angle));
  rotated_pos += center;

  offset = rotated_pos - ((rotated_pos - offset) * factor);
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
  offset.x += pos.x * cosf(angle/180.0f*Math::pi) +  pos.y * sinf(angle/180.0f*Math::pi);
  offset.y -= pos.x * sinf(angle/180.0f*Math::pi) -  pos.y * cosf(angle/180.0f*Math::pi);
}

Vector2f
ViewerState::screen2world(const Vector2i& pos) const
{
  return (Vector2f(pos) - offset) / scale;
}

Rectf
ViewerState::screen2world(const Rect& rect) const
{
  return Rectf((static_cast<float>(rect.left)   - offset.x) / scale,
               (static_cast<float>(rect.top)    - offset.y) / scale,
               (static_cast<float>(rect.right)  - offset.x) / scale,
               (static_cast<float>(rect.bottom) - offset.y) / scale);
}

void
ViewerState::zoom_to(const Size& display_, const Rectf& rect)
{
  assert(rect.is_normal());

  Sizef display = display_;

  if ((display.height / display.width) > (rect.get_height() / rect.get_width()))
  { // match width
    scale = display.width / rect.get_width();

    offset.x = -rect.left * scale;
    offset.y = -(rect.top - ((display.height / scale) - rect.get_height()) / 2.0f) * scale;
  }
  else
  { // match height
    scale = display.height / rect.get_height();

    offset.x = -(rect.left - ((display.width / scale) - rect.get_width()) / 2.0f) * scale;
    offset.y = -rect.top  * scale;
  }
}

/* EOF */
