// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_GRID_COMPONENT_HPP
#define HEADER_WINDSTILLE_GUI_GRID_COMPONENT_HPP

#include <memory>

#include <geom/point.hpp>

#include "component.hpp"
#include "field.hpp"

namespace wstgui {

class GridComponent : public Component
{
private:
  struct ComponentBox
  {
    std::unique_ptr<Component> component;
    geom::isize  span;
    geom::ipoint parent;

    ComponentBox() :
      component(),
      span(1, 1),
      parent(-1, -1)
    {}

    ComponentBox(std::unique_ptr<Component> c, const geom::isize& s, const geom::ipoint& parent_ = geom::ipoint(-1, -1)) :
      component(std::move(c)),
      span(s),
      parent(parent_)
    {}

    bool is_simple()  const { return (span.width() == 1 && span.height() == 1); }
    bool has_parent() const { return (parent.x() != -1 || parent.y() != -1); }
  };

  typedef Field<ComponentBox> Grid;
  Grid  grid;
  glm::ivec2 pos;
  bool  child_active;
  float padding;

public:
  GridComponent(int weight, int height, Component* parent);
  ~GridComponent() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

  void pack(std::unique_ptr<Component> component, int x, int y, int colspan = 1, int rowspan = 1);

  void move_up();
  void move_down();

  void move_left();
  void move_right();

  void on_activation() override;

  void set_padding(float p);

private:
  GridComponent (const GridComponent&);
  GridComponent& operator= (const GridComponent&);
};

} // namespace wstgui

#endif

/* EOF */
