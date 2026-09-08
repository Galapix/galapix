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

#include <cmath>
#include <glm/ext.hpp>

#include "galapix/viewer.hpp"

namespace galapix {

ViewerState::ViewerState(Viewer& viewer) :
  m_viewer(viewer),
  scale{1.0},
  angle{0.0},
  offset{0.0, 0.0}
{
}

void
ViewerState::zoom(double factor, Vector2i const& pos)
{
  scale *= factor;

  Vector2d center(static_cast<double>(m_viewer.get_width())  / 2.0,
                  static_cast<double>(m_viewer.get_height()) / 2.0);

  Vector2d rotated_pos(static_cast<double>(pos.x()), static_cast<double>(pos.y()));
  rotated_pos = rotated_pos.as_vec() - center.as_vec();
  rotated_pos = glm::rotate(rotated_pos.as_vec(), glm::radians(-angle));
  rotated_pos = rotated_pos.as_vec() + center.as_vec();

  offset = rotated_pos.as_vec() - ((rotated_pos.as_vec() - offset.as_vec()) * factor);
}

void
ViewerState::zoom(double factor)
{
  zoom(factor, Vector2i(m_viewer.get_width()/2,
                        m_viewer.get_height()/2));
}

void
ViewerState::rotate(double r)
{
  angle += r;
}

void
ViewerState::set_angle(double r)
{
  angle = r;
}

void
ViewerState::set_offset(Vector2d const& o)
{
  offset = o;
}

void
ViewerState::set_offset(Vector2f const& o)
{
  offset = Vector2d(static_cast<double>(o.x()), static_cast<double>(o.y()));
}

void
ViewerState::set_scale(double s)
{
  scale = s;
}

void
ViewerState::move(Vector2d const& pos)
{
  double const rad = angle / 180.0 * glm::pi<double>();
  double const c = std::cos(rad);
  double const s = std::sin(rad);
  offset = Vector2d(offset.x() + (pos.x() * c + pos.y() * s),
                    offset.y() - (pos.x() * s - pos.y() * c));
}

void
ViewerState::move(Vector2f const& pos)
{
  move(Vector2d(static_cast<double>(pos.x()), static_cast<double>(pos.y())));
}

Vector2d
ViewerState::screen2world(Vector2i const& pos) const
{
  return (Vector2d(static_cast<double>(pos.x()), static_cast<double>(pos.y())).as_vec()
          - offset.as_vec()) / scale;
}

Rectd
ViewerState::screen2world(Rect const& rect) const
{
  return Rectd((static_cast<double>(rect.left())   - offset.x()) / scale,
               (static_cast<double>(rect.top())    - offset.y()) / scale,
               (static_cast<double>(rect.right())  - offset.x()) / scale,
               (static_cast<double>(rect.bottom()) - offset.y()) / scale);
}

Vector2f
ViewerState::screen2world_f(Vector2i const& pos) const
{
  Vector2d const p = screen2world(pos);
  return Vector2f(static_cast<float>(p.x()), static_cast<float>(p.y()));
}

Rectf
ViewerState::screen2world_f(Rect const& rect) const
{
  Rectd const r = screen2world(rect);
  return Rectf(static_cast<float>(r.left()), static_cast<float>(r.top()),
               static_cast<float>(r.right()), static_cast<float>(r.bottom()));
}

void
ViewerState::zoom_to(Size const& display_, Rectf const& rect)
{
  zoom_to(display_, Rectd(static_cast<double>(rect.left()),
                          static_cast<double>(rect.top()),
                          static_cast<double>(rect.right()),
                          static_cast<double>(rect.bottom())));
}

void
ViewerState::zoom_to(Size const& display_, Rectd const& rect)
{
  assert(rect);

  Sized display(static_cast<double>(display_.width()),
                static_cast<double>(display_.height()));

  if ((display.height() / display.width()) > (rect.height() / rect.width()))
  {
    scale = display.width() / rect.width();

    offset = Vector2d(-rect.left() * scale,
                      -(rect.top() - ((display.height() / scale) - rect.height()) / 2.0) * scale);
  }
  else
  {
    scale = display.height() / rect.height();

    offset = Vector2d(-(rect.left() - ((display.width() / scale) - rect.width()) / 2.0) * scale,
                      -rect.top()  * scale);
  }
}

} // namespace galapix

/* EOF */
