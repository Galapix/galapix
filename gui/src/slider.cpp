// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/slider.hpp>

#include <surf/color.hpp>
#include <wstdisplay/canvas.hpp>
#include <wstinput/controller.hpp>

#include <wstgui/controller_def.hpp>
#include <wstgui/list_view.hpp>

namespace wstgui {

Slider::Slider(Component* parent_)
  : Component(parent_),
    min(0),
    max(100),
    step(10),
    pos (50),
    horizontal(true)
{
}

Slider::~Slider()
{
}

int
Slider::set_minimum(int min_)
{
  return min = min_;
}

int
Slider::set_maximum(int max_)
{
  return max = max_;
}

void
Slider::set_range(int min_, int max_)
{
  min = min_;
  max = max_;
}

void
Slider::set_step(int step_)
{
  step = step_;
}

void
Slider::draw(wstdisplay::Canvas& canvas)
{
  canvas.fill_rect(m_geometry, surf::Color(0.0f, 0.0f, 0.0f, 0.5f));
  canvas.draw_rect(m_geometry, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

  float width = 30.0f;
  geom::frect slider_rect(glm::vec2(m_geometry.left() + (m_geometry.width() - width) * (static_cast<float>(pos-min)/static_cast<float>(max - min)),
                                    m_geometry.top() + 2.0f),
                          geom::fsize(width, m_geometry.height() - 4.0f));
  if (is_active())
  {
    canvas.fill_rect(slider_rect, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));
    canvas.draw_rect(slider_rect, surf::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }
  else
  {
    canvas.draw_rect(slider_rect, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));
  }
}

void
Slider::update(float /*delta*/, const Controller& controller)
{
  for(auto i = controller.get_events().begin(); i != controller.get_events().end(); ++i)
  {
    if (i->type == wstinput::BUTTON_EVENT && i->button.down)
    {
      if (i->button.name == OK_BUTTON || i->button.name == ENTER_BUTTON)
      {

      }
      else if (i->button.name == CANCEL_BUTTON || i->button.name == ESCAPE_BUTTON)
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
          pos -= step;
          if (pos < min)
            pos = min;
        }
        else if (i->axis.pos > 0)
        {
          pos += step;
          if (pos > max)
            pos = max;
        }
      }
    }
  }
}

int
Slider::get_pos() const
{
  return pos;
}

void
Slider::set_pos(int pos_)
{
  pos = pos_;
}

} // namespace wstgui

/* EOF */
