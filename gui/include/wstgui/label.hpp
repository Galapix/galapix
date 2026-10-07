// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_LABEL_HPP
#define HEADER_WINDSTILLE_GUI_LABEL_HPP

#include "component.hpp"

namespace wstgui {

/** */
class Label : public Component
{
private:
  std::string label;

public:
  Label(const std::string& label, Component* parent);
  ~Label() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

private:
  Label (const Label&);
  Label& operator= (const Label&);
};

} // namespace wstgui

#endif

/* EOF */
