// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <wstinput/controller.hpp>
#include <wstinput/controller_description.hpp>
#include <wstinput/input_manager.hpp>

using namespace wstinput;

namespace {

enum { MENU_DOWN = 0, JUMP = 1, X_AXIS = 2 };

ControllerDescription make_description()
{
  ControllerDescription desc;
  desc.add_button("menu-down-button", MENU_DOWN);
  desc.add_button("jump-button", JUMP);
  desc.add_axis("x-axis", X_AXIS);
  return desc;
}

int count_button_events(Controller const& controller, int name)
{
  int count = 0;
  for (InputEvent const& event : controller.get_events()) {
    if (event.type == BUTTON_EVENT && event.button.name == name) {
      count += 1;
    }
  }
  return count;
}

} // namespace

TEST(InputBindingsTest, axis_button_reports_changes_only)
{
  InputManagerSDL manager(make_description());
  InputBindings& bindings = manager.bindings();
  bindings.bind_joystick_axis_button(MENU_DOWN, 0, 1, false);

  Controller controller(3);
  // stick noise and a held stick must not repeat the menu step
  for (int value : {100, 2000, 20000, 25000, 32000, 20000, 3000, 0}) {
    bindings.dispatch_joystick_axis(0, 1, value, controller);
  }
  EXPECT_EQ(count_button_events(controller, MENU_DOWN), 2);
  EXPECT_FALSE(controller.get_button_state(MENU_DOWN));
}

TEST(InputBindingsTest, axis_dead_zone)
{
  InputManagerSDL manager(make_description());
  InputBindings& bindings = manager.bindings();
  bindings.bind_joystick_axis(X_AXIS, 0, 0, false);

  Controller controller(3);
  bindings.dispatch_joystick_axis(0, 0, 5000, controller);
  EXPECT_EQ(controller.get_axis_state(X_AXIS, false), 0.0f);

  bindings.dispatch_joystick_axis(0, 0, 16384, controller);
  EXPECT_FLOAT_EQ(controller.get_axis_state(X_AXIS, false), 0.5f);

  bindings.set_dead_zone(0);
  bindings.dispatch_joystick_axis(0, 0, 5000, controller);
  EXPECT_GT(controller.get_axis_state(X_AXIS, false), 0.0f);
}

TEST(InputBindingsTest, buttons_by_device)
{
  InputManagerSDL manager(make_description());
  InputBindings& bindings = manager.bindings();
  bindings.bind_joystick_button(JUMP, 1, 0);

  Controller controller(3);
  bindings.dispatch_joystick_button(0, 0, true, controller);
  EXPECT_FALSE(controller.get_button_state(JUMP));

  bindings.dispatch_joystick_button(1, 0, true, controller);
  EXPECT_TRUE(controller.get_button_state(JUMP));
}

/* EOF */
