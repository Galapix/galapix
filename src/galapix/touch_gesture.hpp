// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_GALAPIX_TOUCH_GESTURE_HPP
#define HEADER_GALAPIX_GALAPIX_TOUCH_GESTURE_HPP

#include <cstdint>
#include <optional>
#include <vector>

#include "math/vector2f.hpp"

namespace galapix {

/** View change caused by one finger move: the content under \a from
    moves to \a to, scaled by \a zoom and rotated by \a angle degrees
    around that point. */
struct TouchDelta
{
  Vector2f from = {};
  Vector2f to = {};
  float zoom = 1.0f;
  float angle = 0.0f;
};

/** Multi-touch pan, pinch zoom and twist rotate, in screen coordinates.

    Every finger move is turned into a delta of the finger centroid
    (pan), of the spread of the first two fingers (zoom) and of their
    angle (rotate). As only the moved finger changes between before and
    after, putting down or lifting a finger never makes the view jump.

    Rotation stays locked until the fingers have twisted by
    kRotateThreshold degrees, so a plain pinch does not tilt the view. */
class TouchGesture
{
public:
  static constexpr float kRotateThreshold = 10.0f;

  TouchGesture();

  void finger_down(std::int64_t id, Vector2f const& pos);
  std::optional<TouchDelta> finger_motion(std::int64_t id, Vector2f const& pos);
  void finger_up(std::int64_t id);
  void cancel();

  bool active() const { return !m_fingers.empty(); }
  bool has_finger(std::int64_t id) const;
  int finger_count() const { return static_cast<int>(m_fingers.size()); }

private:
  struct Finger
  {
    std::int64_t id;
    Vector2f pos;
  };

  Vector2f centroid() const;

  /** Restart the twist that unlocks rotation */
  void reset_twist();

private:
  /** In touch-down order, the first two fingers zoom and rotate */
  std::vector<Finger> m_fingers;
  float m_twist;
  bool m_rotate_unlocked;
};

} // namespace galapix

#endif

/* EOF */
