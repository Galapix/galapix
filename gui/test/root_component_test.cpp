// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <wstgui/controller_def.hpp>
#include <wstgui/group_component.hpp>
#include <wstgui/gui_manager.hpp>
#include <wstgui/label.hpp>
#include <wstgui/menu_component.hpp>
#include <wstgui/root_component.hpp>
#include <wstgui/style.hpp>
#include <wstinput/controller.hpp>

using namespace wstgui;

namespace {

void press(GUIManager& gui, int button)
{
  wstinput::Controller controller(LAST_EVENT);
  controller.add_button_event(button, true);
  gui.update(0.01f, controller);
}

std::unique_ptr<GroupComponent> make_menu(RootComponent* root, bool allow_cancel)
{
  auto group = std::make_unique<GroupComponent>("Menu", root);
  group->pack(std::make_unique<MenuComponent>(allow_cancel, group.get()));
  return group;
}

} // namespace

TEST(RootComponentTest, cancelling_the_menu_closes_the_gui)
{
  Style style(nullptr);
  GUIManager gui(style);
  RootComponent* root = gui.get_root();
  root->add_child(make_menu(root, true));
  // added after the menu, but doesn't take the focus
  root->add_child(std::make_unique<Label>("Hello", root));

  press(gui, MENU_DOWN_BUTTON);
  EXPECT_FALSE(gui.is_finished());

  press(gui, ESCAPE_BUTTON);
  EXPECT_TRUE(gui.is_finished());
}

TEST(RootComponentTest, menu_without_cancel_stays)
{
  Style style(nullptr);
  GUIManager gui(style);
  RootComponent* root = gui.get_root();
  root->add_child(make_menu(root, false));

  press(gui, ESCAPE_BUTTON);
  EXPECT_FALSE(gui.is_finished());
}

TEST(RootComponentTest, gui_without_focus_stays)
{
  Style style(nullptr);
  GUIManager gui(style);
  RootComponent* root = gui.get_root();
  root->add_child(std::make_unique<Label>("Hello", root));

  press(gui, ESCAPE_BUTTON);
  EXPECT_FALSE(gui.is_finished());
}

/* EOF */
