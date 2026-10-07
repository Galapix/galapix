// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_BUTTON_HPP
#define HEADER_WINDSTILLE_GUI_BUTTON_HPP

#include "component.hpp"

namespace wstgui {

class Button : public Component
{
private:
  std::string label;

public:
  Button(const std::string& label, Component* parent);
  ~Button() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

private:
  Button (const Button&);
  Button& operator= (const Button&);
};

} // namespace wstgui

#endif

/* EOF */
