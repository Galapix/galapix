// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_VIEW_HPP
#define HEADER_WSTDISPLAY_VIEW_HPP

#include <glm/glm.hpp>
#include <geom/point.hpp>
#include <geom/rect.hpp>
#include <geom/size.hpp>

namespace wstdisplay {

/** A 2D camera: maps world coordinates to the pixels of a render
    target of the given size. The camera looks at pos, which ends up
    in the center of the target, zooms by zoom and rotates the world
    by rotation degrees around the center of the target. */
class View final
{
public:
  View();
  explicit View(geom::isize const& size);

  void set_size(geom::isize const& size) { m_size = size; }
  geom::isize get_size() const { return m_size; }

  /** The world position shown in the center of the target */
  void set_pos(geom::fpoint const& pos) { m_pos = pos; }
  geom::fpoint get_pos() const { return m_pos; }

  void set_zoom(float zoom) { m_zoom = zoom; }

  /** Set the zoom while keeping the world position under the screen
      position \a screen_pos in place, e.g. the mouse pointer */
  void set_zoom(geom::fpoint const& screen_pos, float zoom);
  float get_zoom() const { return m_zoom; }

  void set_rotation(float degrees) { m_rotation = degrees; }
  float get_rotation() const { return m_rotation; }

  /** Zoom and move so that \a rect is fully visible */
  void zoom_to(geom::frect const& rect);

  /** World to target pixel coordinates */
  glm::mat4 get_matrix() const;

  /** Target pixel coordinates to clip space, (0,0) is top left */
  glm::mat4 get_projection() const;

  geom::fpoint screen_to_world(geom::fpoint const& pos) const;
  geom::fpoint world_to_screen(geom::fpoint const& pos) const;

  /** The visible area in world coordinates (bounding box when rotated) */
  geom::frect get_clip_rect() const;

private:
  geom::isize m_size;
  geom::fpoint m_pos;
  float m_zoom;
  float m_rotation;
};

} // namespace wstdisplay

#endif

/* EOF */
