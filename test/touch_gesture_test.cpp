// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <gtest/gtest.h>

#include <glm/ext.hpp>

#include "galapix/system.hpp"
#include "galapix/touch_gesture.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"

using namespace galapix;

namespace {

class FakeSystem final : public System
{
public:
  void launch_viewer(Workspace&, Options&) override {}
  bool requires_command_line_args() override { return false; }
  void trigger_redraw() override {}
  void set_trackball_mode(bool) override {}
};

/** Inverse of ViewerState::screen2world() including the view rotation */
Vector2f world2screen(ViewerState const& state, Size const& size, Vector2f const& world)
{
  glm::vec2 const center(static_cast<float>(size.width()) / 2.0f,
                         static_cast<float>(size.height()) / 2.0f);
  return Vector2f(center + glm::rotate(state.get_offset().as_vec() + state.get_scale() * world.as_vec() - center,
                                       glm::radians(state.get_angle())));
}

void expect_near(Vector2f const& a, Vector2f const& b)
{
  EXPECT_NEAR(a.x(), b.x(), 0.01f);
  EXPECT_NEAR(a.y(), b.y(), 0.01f);
}

} // namespace

TEST(TouchGestureTest, one_finger_pans)
{
  TouchGesture gesture;
  gesture.finger_down(1, Vector2f(100.0f, 100.0f));
  auto delta = gesture.finger_motion(1, Vector2f(130.0f, 90.0f));
  ASSERT_TRUE(delta);
  expect_near(delta->from, Vector2f(100.0f, 100.0f));
  expect_near(delta->to, Vector2f(130.0f, 90.0f));
  EXPECT_FLOAT_EQ(delta->zoom, 1.0f);
  EXPECT_FLOAT_EQ(delta->angle, 0.0f);

  gesture.finger_up(1);
  EXPECT_FALSE(gesture.active());
  EXPECT_FALSE(gesture.finger_motion(1, Vector2f(0.0f, 0.0f)));
}

TEST(TouchGestureTest, two_fingers_pinch_zoom_around_centroid)
{
  TouchGesture gesture;
  gesture.finger_down(1, Vector2f(100.0f, 100.0f));
  gesture.finger_down(2, Vector2f(200.0f, 100.0f));

  auto delta = gesture.finger_motion(2, Vector2f(300.0f, 100.0f));
  ASSERT_TRUE(delta);
  EXPECT_FLOAT_EQ(delta->zoom, 2.0f);
  EXPECT_FLOAT_EQ(delta->angle, 0.0f);
  expect_near(delta->from, Vector2f(150.0f, 100.0f));
  expect_near(delta->to, Vector2f(200.0f, 100.0f));
}

TEST(TouchGestureTest, rotation_unlocks_after_threshold)
{
  TouchGesture gesture;
  gesture.finger_down(1, Vector2f(0.0f, 0.0f));
  gesture.finger_down(2, Vector2f(100.0f, 0.0f));

  auto rotated = [](float degrees) {
    return Vector2f(100.0f * std::cos(glm::radians(degrees)),
                    100.0f * std::sin(glm::radians(degrees)));
  };

  // a small twist while pinching does not rotate
  auto delta = gesture.finger_motion(2, rotated(5.0f));
  ASSERT_TRUE(delta);
  EXPECT_FLOAT_EQ(delta->angle, 0.0f);

  // crossing the threshold unlocks, the following twist rotates
  delta = gesture.finger_motion(2, rotated(12.0f));
  ASSERT_TRUE(delta);
  EXPECT_FLOAT_EQ(delta->angle, 0.0f);

  delta = gesture.finger_motion(2, rotated(20.0f));
  ASSERT_TRUE(delta);
  EXPECT_NEAR(delta->angle, 8.0f, 0.001f);

  // a new pair of fingers has to unlock again
  gesture.finger_up(1);
  gesture.finger_down(3, Vector2f(0.0f, 0.0f));
  delta = gesture.finger_motion(2, rotated(25.0f));
  ASSERT_TRUE(delta);
  EXPECT_FLOAT_EQ(delta->angle, 0.0f);
}

TEST(TouchGestureTest, third_finger_only_pans)
{
  TouchGesture gesture;
  gesture.finger_down(1, Vector2f(0.0f, 0.0f));
  gesture.finger_down(2, Vector2f(100.0f, 0.0f));
  gesture.finger_down(3, Vector2f(50.0f, 90.0f));

  auto delta = gesture.finger_motion(3, Vector2f(50.0f, 120.0f));
  ASSERT_TRUE(delta);
  EXPECT_FLOAT_EQ(delta->zoom, 1.0f);
  expect_near(delta->from, Vector2f(50.0f, 30.0f));
  expect_near(delta->to, Vector2f(50.0f, 40.0f));
}

TEST(TouchGestureTest, pinch_keeps_content_under_fingers)
{
  FakeSystem system;
  Workspace workspace;
  Viewer viewer(system, &workspace);
  Size const size(800, 600);
  viewer.reshape(size);
  viewer.get_state().set_angle(30.0f);
  viewer.get_state().set_scale(0.5f);

  // world points initially under the two fingers
  Vector2f const p1(100.0f, 200.0f);
  Vector2f const p2(500.0f, 300.0f);
  ViewerState const& state = viewer.get_state();

  glm::vec2 const center(400.0f, 300.0f);
  auto screen2world = [&](Vector2f const& p) {
    return Vector2f((glm::rotate(p.as_vec() - center, glm::radians(-state.get_angle()))
                     + center - state.get_offset().as_vec()) / state.get_scale());
  };
  Vector2f const world1 = screen2world(p1);
  Vector2f const world2 = screen2world(p2);

  viewer.on_finger_down(1, p1);
  viewer.on_finger_down(2, p2);

  // spread apart and twist well past the rotate threshold, in steps
  Vector2f q1 = p1;
  Vector2f q2 = p2;
  for (int i = 1; i <= 10; ++i) {
    float const t = static_cast<float>(i) / 10.0f;
    q1 = Vector2f(100.0f - 50.0f * t, 200.0f - 150.0f * t);
    q2 = Vector2f(500.0f + 100.0f * t, 300.0f + 200.0f * t);
    viewer.on_finger_motion(1, q1);
    viewer.on_finger_motion(2, q2);
  }
  EXPECT_NE(state.get_angle(), 30.0f);

  // Rotation is locked for the first few steps, so only the centroid
  // is pinned exactly; the fingers drift a little against the content.
  Vector2f const centroid((q1.as_vec() + q2.as_vec()) / 2.0f);
  Vector2f const world_centroid((world1.as_vec() + world2.as_vec()) / 2.0f);
  expect_near(world2screen(state, size, world_centroid), centroid);

  // Once unlocked, further motion keeps both fingers glued to the content
  Vector2f const pinned1 = screen2world(q1);
  Vector2f const pinned2 = screen2world(q2);
  q1 = Vector2f(0.0f, 100.0f);
  q2 = Vector2f(700.0f, 400.0f);
  viewer.on_finger_motion(1, q1);
  viewer.on_finger_motion(2, q2);
  expect_near(world2screen(state, size, pinned1), q1);
  expect_near(world2screen(state, size, pinned2), q2);

  viewer.on_finger_up(1);
  viewer.on_finger_up(2);
  EXPECT_FALSE(viewer.touch_active());
}

/* EOF */
