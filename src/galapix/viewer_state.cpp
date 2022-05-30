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

#include <glm/ext.hpp>

#include "galapix/viewer.hpp"

namespace galapix {

ViewerState::ViewerState(Viewer& viewer) :
  m_viewer(viewer),
  scale{1.0f},
  angle{0.0f},
  offset{0.0f, 0.0f}
{
}

void
ViewerState::zoom(float factor, Vector2i const& pos)
{
  scale *= factor;

  Vector2f center(static_cast<float>(m_viewer.get_width())  / 2.0f,
                  static_cast<float>(m_viewer.get_height()) / 2.0f);

  Vector2f rotated_pos(pos);
  rotated_pos = rotated_pos.as_vec() - center.as_vec();
  rotated_pos = glm::rotate(rotated_pos.as_vec(), glm::radians(-angle));
  rotated_pos = rotated_pos.as_vec() + center.as_vec();

  offset = rotated_pos.as_vec() - ((rotated_pos.as_vec() - offset.as_vec()) * factor);
}

void
ViewerState::zoom(float factor)
{
  zoom(factor, Vector2i(m_viewer.get_width()/2, // FIXME: Little ugly, isn't it?
                        m_viewer.get_height()/2));
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
ViewerState::set_offset(Vector2f const& o)
{
  offset = o;
}

void
ViewerState::set_scale(float s)
{
  scale = s;
}

void
ViewerState::move(Vector2f const& pos)
{
  // FIXME: Implement a proper 2D Matrix instead of this hackery
  offset = Vector2f(offset.x() + (pos.x() * cosf(angle/180.0f*glm::pi<float>()) + pos.y() * sinf(angle/180.0f*glm::pi<float>())),
                    offset.y() - (pos.x() * sinf(angle/180.0f*glm::pi<float>()) - pos.y() * cosf(angle/180.0f*glm::pi<float>())));
}

Vector2f
ViewerState::screen2world(Vector2i const& pos) const
{
  return (Vector2f(pos).as_vec() - offset.as_vec()) / scale;
}

Rectf
ViewerState::screen2world(Rect const& rect) const
{
  return Rectf((static_cast<float>(rect.left())   - offset.x()) / scale,
               (static_cast<float>(rect.top())    - offset.y()) / scale,
               (static_cast<float>(rect.right())  - offset.x()) / scale,
               (static_cast<float>(rect.bottom()) - offset.y()) / scale);
}

void
ViewerState::zoom_to(Size const& display_, Rectf const& rect)
{
  assert(rect);

  Sizef display = Sizef(display_);

  if ((display.height() / display.width()) > (rect.height() / rect.width()))
  { // match width
    scale = display.width() / rect.width();

    offset = Vector2f(-rect.left() * scale,
                      -(rect.top() - ((display.height() / scale) - rect.height()) / 2.0f) * scale);
  }
  else
  { // match height
    scale = display.height() / rect.height();

    offset = Vector2f(-(rect.left() - ((display.width() / scale) - rect.width()) / 2.0f) * scale,
                      -rect.top()  * scale);
  }
}

} // namespace galapix

/* EOF */
