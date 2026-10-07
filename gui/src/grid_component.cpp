// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/grid_component.hpp>

#include <iostream>

#include <logmich/log.hpp>
#include <surf/color.hpp>
#include <wstdisplay/canvas.hpp>
#include <wstinput/controller.hpp>

#include <wstgui/controller_def.hpp>

namespace wstgui {

GridComponent::GridComponent(int weight, int height, Component* parent_)
  : Component(parent_),
    grid(weight, height),
    pos(0, 0),
    child_active(false),
    padding(15.0f)
{
}

GridComponent::~GridComponent()
{
}

void
GridComponent::draw(wstdisplay::Canvas& canvas)
{
  canvas.fill_rect(m_geometry, surf::Color(0.0f, 0.0f, 0.0f, 0.5f));
  canvas.draw_rect(m_geometry, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

  for(int y = 0; y < grid.get_height(); ++y)
    for(int x = 0; x < grid.get_width(); ++x)
    {
      if (grid(x, y).component)
      {
        if (x == pos.x && y == pos.y)
          canvas.fill_rect(grid(x, y).component->geometry(), surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

        grid(x, y).component->draw(canvas);
      }
    }
}

void
GridComponent::update(float delta, const Controller& controller)
{
  auto get_component = [this]{
    if (grid(pos.x, pos.y).has_parent()) {
      geom::ipoint parent_pos = grid(pos.x, pos.y).parent;
      return grid(parent_pos.x(), parent_pos.y()).component.get();
    } else {
      return grid(pos.x, pos.y).component.get();
    }
  };

  if (child_active && !get_component()->is_active())
  {
    child_active = false;
  }
  else if (!child_active)
  {
    for(auto i = controller.get_events().begin();
        i != controller.get_events().end();
        ++i)
    {
      if (i->type == wstinput::BUTTON_EVENT && i->button.down)
      {
        if (i->button.name == OK_BUTTON)
        {
          child_active = true;
          get_component()->set_active(true);
        }
        else if (i->button.name == CANCEL_BUTTON)
        {
          set_active(false);
        }
      }
      else if (i->type == wstinput::AXIS_EVENT)
      {
        if (i->axis.name == X_AXIS)
        {
          if (i->axis.pos < 0)
          {
            move_left();
          }
          else if (i->axis.pos > 0)
          {
            move_right();
          }
        }
        else if (i->axis.name == Y_AXIS)
        {
          if (i->axis.pos < 0)
          {
            move_down();
          }
          else if (i->axis.pos > 0)
          {
            move_up();
          }
        }
      }
    }
  }

  for(int y = 0; y < grid.get_height(); ++y)
    for(int x = 0; x < grid.get_width(); ++x)
    {
      if (grid(x, y).component)
      {
        // give input to current compontent, empty input to the rest
        if (child_active && pos.x == x && pos.y == y)
          grid(x, y).component->update(delta, controller);
        else
          grid(x, y).component->update(delta, Controller());
      }
    }
}

void
GridComponent::move_up()
{
  //grid(pos.x, pos.y).component->set_active(false);

  pos.y += grid(pos.x, pos.y).span.height();
  if (pos.y >= grid.get_height())
    pos.y = 0;

  if (grid(pos.x, pos.y).has_parent())
    pos = grid(pos.x, pos.y).parent.as_vec();

  //grid(pos.x, pos.y).component->set_active(true);
}

void
GridComponent::move_down()
{
  //grid(pos.x, pos.y).component->set_active(false);

  pos.y -= 1;
  if (pos.y < 0)
    pos.y = grid.get_height()-1;

  if (grid(pos.x, pos.y).has_parent())
    pos = grid(pos.x, pos.y).parent.as_vec();

  //grid(pos.x, pos.y).component->set_active(true);
}

void
GridComponent::move_left()
{
  //grid(pos.x, pos.y).component->set_active(false);

  pos.x -= 1;
  if (pos.x < 0)
    pos.x = grid.get_width()-1;

  if (grid(pos.x, pos.y).has_parent())
    pos = grid(pos.x, pos.y).parent.as_vec();

  //grid(pos.x, pos.y).component->set_active(true);
}

void
GridComponent::move_right()
{
  //grid(pos.x, pos.y).component->set_active(false);

  pos.x += grid(pos.x, pos.y).span.width();
  if (pos.x >= grid.get_width())
    pos.x = 0;

  if (grid(pos.x, pos.y).has_parent())
    pos = grid(pos.x, pos.y).parent.as_vec();

  //grid(pos.x, pos.y).component->set_active(true);
}

void
GridComponent::pack(std::unique_ptr<Component> component, int x, int y, int colspan, int rowspan)
{
  assert(x >= 0);
  assert(y >= 0);
  assert(x < grid.get_width());
  assert(y < grid.get_height());

  if (grid(x, y).component || grid(x, y).has_parent()) {
    std::cout << "Warning component already at: " << x << ", " << y << ", ignoring" << std::endl;
    return;
  }

  geom::frect rect_ = geometry();

  if (colspan == 1 && rowspan == 1)
  {
    grid(x, y) = ComponentBox(std::move(component), geom::isize(colspan, rowspan));
  }
  else
  {
    for(int iy = 0; iy < rowspan; ++iy) {
      for(int ix = 0; ix < colspan; ++ix) {
        grid(x + ix, y + iy) = ComponentBox({}, geom::isize(0, 0), geom::ipoint(x, y));
      }
    }

    grid(x, y) = ComponentBox(std::move(component), geom::isize(colspan, rowspan));
  }

  grid(x, y).component->set_geometry(
    geom::frect(glm::vec2(rect_.left() + static_cast<float>(x) * (rect_.width()  / static_cast<float>(grid.get_width()))  + padding,
                          rect_.top()  + static_cast<float>(y) * (rect_.height() / static_cast<float>(grid.get_height())) + padding),
                geom::fsize((rect_.width()  / static_cast<float>(grid.get_width()))  * static_cast<float>(colspan) - 2.0f * padding,
                            (rect_.height() / static_cast<float>(grid.get_height())) * static_cast<float>(rowspan) - 2.0f * padding)));
}

void
GridComponent::set_padding(float p)
{
  padding = p;
}

void
GridComponent::on_activation()
{
  //grid(pos.x, pos.y).component->set_active(true);
}

} // namespace wstgui

/* EOF */
