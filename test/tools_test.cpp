// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include <gtest/gtest.h>

#include "galapix/components.hpp"
#include "galapix/system.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "tools/tools.hpp"

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

} // namespace

/** Mouse tools against a Viewer without a window: at the initial view
    (scale 1, offset 0) screen and world coordinates are the same. Two
    256x256 placeholder images, a centered at (200,200), b at (600,200). */
class ToolsTest : public ::testing::Test
{
protected:
  ToolsTest() :
    system(),
    workspace(),
    a(workspace.add_image(URL::from_filename("/tmp/a.jpg"), nullptr)),
    b(workspace.add_image(URL::from_filename("/tmp/b.jpg"), nullptr)),
    viewer(system, &workspace)
  {
    viewer.reshape(Size(800, 600));
    workspace.registry().get<Transform>(a).pos = Vector2f(200.0f, 200.0f);
    workspace.registry().get<Transform>(b).pos = Vector2f(600.0f, 200.0f);
  }

  void drag(MouseButton button, Vector2i from, Vector2i to)
  {
    viewer.on_mouse_button_down(from, button);
    viewer.on_mouse_motion(to, Vector2i(to.x() - from.x(), to.y() - from.y()));
    viewer.on_mouse_button_up(to, button);
  }

  Vector2f pos(entt::entity image) { return workspace.registry().get<Transform>(image).pos; }

  FakeSystem system;
  Workspace workspace;
  entt::entity a;
  entt::entity b;
  Viewer viewer;
};

TEST_F(ToolsTest, move_tool_click_selects_and_drags)
{
  viewer.set_move_resize_tool();

  drag(MouseButton::LEFT, Vector2i(200, 200), Vector2i(230, 210));
  EXPECT_EQ(workspace.get_selection(), (std::vector<entt::entity>{a}));
  EXPECT_EQ(pos(a), Vector2f(230.0f, 210.0f));
  EXPECT_EQ(pos(b), Vector2f(600.0f, 200.0f));

  // click again toggles it off
  drag(MouseButton::LEFT, Vector2i(230, 210), Vector2i(230, 210));
  EXPECT_TRUE(workspace.selection_empty());
}

TEST_F(ToolsTest, move_tool_box_select)
{
  viewer.set_move_resize_tool();

  drag(MouseButton::LEFT, Vector2i(20, 20), Vector2i(780, 380));
  EXPECT_EQ(workspace.get_selection(), (std::vector<entt::entity>{a, b}));
  // the box only selects, nothing moved
  EXPECT_EQ(pos(a), Vector2f(200.0f, 200.0f));

  // a box on empty space clears the selection
  drag(MouseButton::LEFT, Vector2i(20, 500), Vector2i(40, 520));
  EXPECT_TRUE(workspace.selection_empty());
}

TEST_F(ToolsTest, resize_tool_scales_selection)
{
  viewer.set_move_resize_tool();
  workspace.select_images({a, b});

  // center (400,200): dragging from 100 to 200 away doubles the size
  drag(MouseButton::RIGHT, Vector2i(500, 200), Vector2i(600, 200));
  EXPECT_FLOAT_EQ(workspace.registry().get<Transform>(a).scale, 2.0f);
  EXPECT_EQ(pos(a), Vector2f(0.0f, 200.0f));
  EXPECT_EQ(pos(b), Vector2f(800.0f, 200.0f));
}

TEST_F(ToolsTest, zoom_rect_tool)
{
  viewer.set_zoom_tool();

  drag(MouseButton::LEFT, Vector2i(100, 100), Vector2i(300, 250));
  EXPECT_FLOAT_EQ(viewer.get_state().get_scale(), 4.0f);
  Vector2f const top_left = viewer.get_state().screen2world(Vector2i(0, 0));
  EXPECT_FLOAT_EQ(top_left.x(), 100.0f);
  EXPECT_FLOAT_EQ(top_left.y(), 100.0f);
}

TEST_F(ToolsTest, pan_tool_on_middle_button)
{
  viewer.set_pan_tool();

  drag(MouseButton::MIDDLE, Vector2i(400, 300), Vector2i(410, 300));
  // pan speed: 4 view pixels per mouse pixel
  EXPECT_FLOAT_EQ(viewer.get_state().get_offset().x(), 40.0f);

  // no button held: no pan
  viewer.on_mouse_motion(Vector2i(420, 300), Vector2i(10, 0));
  EXPECT_FLOAT_EQ(viewer.get_state().get_offset().x(), 40.0f);
}

TEST_F(ToolsTest, tool_change_ends_drag)
{
  viewer.set_move_resize_tool();
  viewer.on_mouse_button_down(Vector2i(200, 200), MouseButton::LEFT);
  viewer.set_pan_tool();

  // the move tool's drag is gone, the motion moves nothing
  viewer.on_mouse_motion(Vector2i(250, 200), Vector2i(50, 0));
  EXPECT_EQ(pos(a), Vector2f(200.0f, 200.0f));
  EXPECT_FALSE(viewer.is_active());
}

/* EOF */
