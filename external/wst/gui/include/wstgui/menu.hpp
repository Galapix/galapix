// SPDX-FileCopyrightText: 2009 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_MENU_HPP
#define HEADER_WINDSTILLE_GUI_MENU_HPP

#include <functional>

#include "fwd.hpp"

namespace wstgui {

/**
 *  Little helper class around MenuItem and MenuComponent to reduce code clutter.
 */
class Menu
{
public:
  /**
   *  Construct a Menu, if \a parent is given no GUIManager will be
   *  created and show() will not work, use create_group() instead and
   *  add the result to your parent component.
   */
  Menu(const std::string& name, const geom::frect& rect, Style& style,
       bool allow_cancel = true, Component* parent = nullptr);
  ~Menu();

  EnumMenuItem& add_enum(const std::string& name,
                         int index,
                         const std::function<void (int)>& callback = std::function<void (int)>());

  void add_slider(const std::string& name,
                  int value, int mix_value, int max_value, int step,
                  const std::function<void (int)>& callback = std::function<void (int)>());

  void add_button(const std::string& name,
                  const std::function<void ()>& callback = std::function<void ()>());

  RootComponent*  get_root() const;

  void show(ScreenManager& screen_manager);

  /** Finish the menu and return it as a component to add to \a parent
      given in the constructor, no more items can be added afterwards */
  std::unique_ptr<GroupComponent> create_group();

private:
  /** Shrink the group around the preferred size of the menu */
  void layout();

private:
  std::unique_ptr<GUIManager> m_manager;
  std::unique_ptr<GroupComponent> m_group;
  std::unique_ptr<MenuComponent> m_menu;

private:
  Menu (const Menu&);
  Menu& operator= (const Menu&);
};

} // namespace wstgui

#endif

/* EOF */
