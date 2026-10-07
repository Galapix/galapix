// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/view.hpp>

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

namespace wstdisplay {

View::View() :
  View(geom::isize(1, 1))
{
}

View::View(geom::isize const& size) :
  m_size(size),
  m_pos(static_cast<float>(size.width()) / 2.0f, static_cast<float>(size.height()) / 2.0f),
  m_zoom(1.0f),
  m_rotation(0.0f)
{
}

glm::mat4
View::get_matrix() const
{
  glm::mat4 matrix(1.0f);
  matrix = glm::translate(matrix, glm::vec3(static_cast<float>(m_size.width()) / 2.0f,
                                            static_cast<float>(m_size.height()) / 2.0f, 0.0f));
  matrix = glm::rotate(matrix, glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  matrix = glm::scale(matrix, glm::vec3(m_zoom, m_zoom, 1.0f));
  matrix = glm::translate(matrix, glm::vec3(-m_pos.x(), -m_pos.y(), 0.0f));
  return matrix;
}

glm::mat4
View::get_projection() const
{
  return glm::ortho(0.0f, static_cast<float>(m_size.width()),
                    static_cast<float>(m_size.height()), 0.0f,
                    -1.0f, 1.0f);
}

geom::fpoint
View::screen_to_world(geom::fpoint const& pos) const
{
  glm::vec4 const p = glm::inverse(get_matrix()) * glm::vec4(pos.x(), pos.y(), 0.0f, 1.0f);
  return geom::fpoint(p.x, p.y);
}

geom::fpoint
View::world_to_screen(geom::fpoint const& pos) const
{
  glm::vec4 const p = get_matrix() * glm::vec4(pos.x(), pos.y(), 0.0f, 1.0f);
  return geom::fpoint(p.x, p.y);
}

void
View::set_zoom(geom::fpoint const& screen_pos, float zoom)
{
  geom::fpoint const before = screen_to_world(screen_pos);
  m_zoom = zoom;
  geom::fpoint const after = screen_to_world(screen_pos);
  m_pos = geom::fpoint(m_pos.x() + before.x() - after.x(),
                       m_pos.y() + before.y() - after.y());
}

void
View::zoom_to(geom::frect const& rect)
{
  m_pos = geom::fpoint((rect.left() + rect.right()) / 2.0f,
                       (rect.top() + rect.bottom()) / 2.0f);
  m_zoom = std::min(static_cast<float>(m_size.width()) / rect.width(),
                    static_cast<float>(m_size.height()) / rect.height());
}

geom::frect
View::get_clip_rect() const
{
  float const w = static_cast<float>(m_size.width());
  float const h = static_cast<float>(m_size.height());
  geom::fpoint const corners[] = {
    screen_to_world(geom::fpoint(0.0f, 0.0f)),
    screen_to_world(geom::fpoint(w, 0.0f)),
    screen_to_world(geom::fpoint(w, h)),
    screen_to_world(geom::fpoint(0.0f, h))
  };

  float left = corners[0].x();
  float right = corners[0].x();
  float top = corners[0].y();
  float bottom = corners[0].y();
  for (auto const& p : corners) {
    left = std::min(left, p.x());
    right = std::max(right, p.x());
    top = std::min(top, p.y());
    bottom = std::max(bottom, p.y());
  }
  return geom::frect(left, top, right, bottom);
}

} // namespace wstdisplay

/* EOF */
