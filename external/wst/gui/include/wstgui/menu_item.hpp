// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_MENU_ITEM_HPP
#define HEADER_WINDSTILLE_GUI_MENU_ITEM_HPP

#include <string>
#include <vector>

#include <wstsystem/signal.hpp>
#include <wstdisplay/fwd.hpp>
#include <geom/geom.hpp>

namespace wstgui {

class MenuComponent;

class MenuItem
{
protected:
  MenuComponent* parent;
  std::string label;
  float fade_timer;

public:
  MenuItem(MenuComponent* parent_, const std::string& label_);
  virtual ~MenuItem() {}

  virtual void incr() =0;
  virtual void decr() =0;

  virtual void click() =0;

  virtual void draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active);
  virtual void update(float delta);

private:
  MenuItem(const MenuItem&);
  MenuItem& operator=(const MenuItem&);
};

class EnumMenuItem : public MenuItem
{
private: // FIXME: Convert this into a generic enum/value slider
  struct EnumValue {
    std::string label;
    int         value;

    EnumValue()
      : label(),
        value()
    {}
  };

  int index;
  std::vector<EnumValue> labels;
  wstsystem::Signal<void (int)> on_change;
public:
  EnumMenuItem(MenuComponent* parent_,
               const std::string& label_, int index_ = 0);

  EnumMenuItem& add_pair(int value, const std::string& label);

  void incr() override;
  void decr() override;
  void click() override {}
  void draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active) override;
  wstsystem::Signal<void (int)>& sig_change() { return on_change; }
};

/** A slider widget for use in volume controls, gamma controls and
    things like that */
class SliderMenuItem : public MenuItem
{
public:
  int value;
  int min_value;
  int max_value;
  int step;
  wstsystem::Signal<void (int)> on_change;
public:
  SliderMenuItem(MenuComponent* parent_,
                 const std::string& label_, int value_, int mix_value_ = 0, int max_value_ = 100, int step = 10);
  void incr() override;
  void decr() override;
  void click() override {}
  void draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active) override;
  wstsystem::Signal<void (int)>& sig_change() { return on_change; }
};

class ButtonMenuItem : public MenuItem
{
public:
  wstsystem::Signal<void ()> on_click;

public:
  ButtonMenuItem(MenuComponent* parent_, const std::string& label_);
  void incr() override {}
  void decr() override {}
  void click() override;
  void draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active) override;
  wstsystem::Signal<void ()>& sig_click() { return on_click; }
};

} // namespace wstgui

#endif

/* EOF */
