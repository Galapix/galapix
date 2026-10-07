// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_TEXT_VIEW_HPP
#define HEADER_WINDSTILLE_GUI_TEXT_VIEW_HPP

#include <wstdisplay/font/text_area.hpp>

#include "component.hpp"

namespace wstgui {

/** */
class TextView : public Component
{
private:
  wstdisplay::TextArea text_area;

public:
  TextView(Component* component);
  ~TextView() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

  void set_geometry(const geom::frect& rect) override;
  void set_text(const std::string& text);
  void set_font(wstdisplay::Font const* font);

private:
  TextView (const TextView&);
  TextView& operator= (const TextView&);
};

} // namespace wstgui

#endif

/* EOF */
