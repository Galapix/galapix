// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_SLIDER_HPP
#define HEADER_WINDSTILLE_GUI_SLIDER_HPP

#include "component.hpp"

namespace wstgui {

/** */
class Slider : public Component
{
private:
  int min;
  int max;
  int step;
  int pos;
  bool horizontal;

public:
  Slider(Component* parent);
  ~Slider() override;

  int  get_pos() const;

  void set_pos(int pos);
  int  set_minimum(int min);
  int  set_maximum(int max);
  void set_range(int min, int max);
  void set_step(int step);

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

private:
  Slider (const Slider&);
  Slider& operator= (const Slider&);
};

} // namespace wstgui

#endif

/* EOF */
