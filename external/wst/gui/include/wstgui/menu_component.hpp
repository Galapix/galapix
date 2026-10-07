// SPDX-FileCopyrightText: 2007 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_MENU_COMPONENT_HPP
#define HEADER_WINDSTILLE_GUI_MENU_COMPONENT_HPP

#include <memory>
#include <vector>

#include <wstdisplay/fwd.hpp>

#include "component.hpp"

namespace wstgui {

class MenuItem;

class MenuComponent : public Component
{
public:
  MenuComponent(bool allow_cancel_, Component* parent);
  ~MenuComponent() override;

  void add_item(std::unique_ptr<MenuItem> item);
  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

  geom::fsize get_prefered_size() const override;

  void set_font(wstdisplay::Font const* font_);
  wstdisplay::Font const* get_font();

  void set_geometry(const geom::frect& rect) override;

private:
  /** Calculate how much height will be needed for the menu */
  float calc_height();

  /** Return the height of a single item */
  float item_height() const;

  void adjust_scroll_offset();

private:
  std::vector<std::unique_ptr<MenuItem>> m_items;

  int m_current_item;
  wstdisplay::Font const* m_font;
  bool m_allow_cancel;

  bool m_scroll_mode;
  int m_scroll_offset;
  int m_num_displayable_items;

private:
  MenuComponent (const MenuComponent&);
  MenuComponent& operator= (const MenuComponent&);
};

} // namespace wstgui

#endif

/* EOF */
