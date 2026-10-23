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

#include "galapix/touch_gesture.hpp"

#include <algorithm>
#include <cmath>

#include <glm/gtc/constants.hpp>

namespace galapix {

namespace {

/** Below this finger spread (in pixels) zoom and angle are too noisy */
constexpr float kMinSpread = 8.0f;

float wrap_degrees(float angle)
{
  while (angle > 180.0f) { angle -= 360.0f; }
  while (angle < -180.0f) { angle += 360.0f; }
  return angle;
}

} // namespace

TouchGesture::TouchGesture() :
  m_fingers(),
  m_twist(0.0f),
  m_rotate_unlocked(false)
{
}

void
TouchGesture::finger_down(std::int64_t id, Vector2f const& pos)
{
  if (has_finger(id)) {
    return;
  }
  m_fingers.push_back(Finger{id, pos});
  if (m_fingers.size() <= 2) {
    reset_twist();
  }
}

std::optional<TouchDelta>
TouchGesture::finger_motion(std::int64_t id, Vector2f const& pos)
{
  auto it = std::find_if(m_fingers.begin(), m_fingers.end(),
                         [id](Finger const& f) { return f.id == id; });
  if (it == m_fingers.end()) {
    return std::nullopt;
  }

  TouchDelta delta;
  delta.from = centroid();

  bool const pinch_finger = m_fingers.size() >= 2 && (it - m_fingers.begin()) < 2;
  glm::vec2 before(0.0f, 0.0f);
  if (pinch_finger) {
    before = m_fingers[1].pos.as_vec() - m_fingers[0].pos.as_vec();
  }

  it->pos = pos;
  delta.to = centroid();

  if (pinch_finger)
  {
    glm::vec2 const after = m_fingers[1].pos.as_vec() - m_fingers[0].pos.as_vec();
    float const spread_before = glm::length(before);
    float const spread_after = glm::length(after);
    if (spread_before >= kMinSpread && spread_after >= kMinSpread)
    {
      delta.zoom = spread_after / spread_before;

      float const twist = wrap_degrees((std::atan2(after.y, after.x) - std::atan2(before.y, before.x))
                                       / glm::pi<float>() * 180.0f);
      if (m_rotate_unlocked) {
        delta.angle = twist;
      } else {
        m_twist += twist;
        m_rotate_unlocked = std::fabs(m_twist) >= kRotateThreshold;
      }
    }
  }

  return delta;
}

void
TouchGesture::finger_up(std::int64_t id)
{
  auto it = std::find_if(m_fingers.begin(), m_fingers.end(),
                         [id](Finger const& f) { return f.id == id; });
  if (it == m_fingers.end()) {
    return;
  }
  bool const pinch_finger = (it - m_fingers.begin()) < 2;
  m_fingers.erase(it);
  if (pinch_finger) {
    // a different pair of fingers now does the pinch
    reset_twist();
  }
}

void
TouchGesture::cancel()
{
  m_fingers.clear();
  reset_twist();
}

bool
TouchGesture::has_finger(std::int64_t id) const
{
  return std::any_of(m_fingers.begin(), m_fingers.end(),
                     [id](Finger const& f) { return f.id == id; });
}

Vector2f
TouchGesture::centroid() const
{
  glm::vec2 sum(0.0f, 0.0f);
  for (Finger const& f : m_fingers) {
    sum += f.pos.as_vec();
  }
  return Vector2f(sum / static_cast<float>(m_fingers.size()));
}

void
TouchGesture::reset_twist()
{
  m_twist = 0.0f;
  m_rotate_unlocked = false;
}

} // namespace galapix

/* EOF */
