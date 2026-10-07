// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/text_view.hpp>

#include <wstinput/controller.hpp>

#include <wstgui/controller_def.hpp>
#include <wstgui/slider.hpp>

namespace wstgui {

TextView::TextView(Component* component) :
  Component(component),
  text_area(m_parent->get_style().get_font(), geom::frect(), false)
{
}

TextView::~TextView()
{
}

void
TextView::draw(wstdisplay::Canvas& canvas)
{
  text_area.draw(canvas);
}

void
TextView::update(float delta, const Controller& controller)
{
  text_area.update(delta);

  text_area.set_scroll_offset(text_area.get_scroll_offset() + 500.0f * controller.get_axis_state(Y2_AXIS) * delta);

  for(auto i = controller.get_events().begin(); i != controller.get_events().end(); ++i)
  {
    if (i->type == wstinput::BUTTON_EVENT && i->button.down)
    {
      if (i->button.name == OK_BUTTON || i->button.name == ENTER_BUTTON)
      {
        set_active(false);
      }
      else if (i->button.name == CANCEL_BUTTON || i->button.name == ESCAPE_BUTTON)
      {
        set_active(false);
      }
    }
  }
}

void
TextView::set_geometry(const geom::frect& rect_)
{
  Component::set_geometry(rect_);
  text_area.set_rect(geom::grow(rect_, -16.0f, -4.0f));
}

void
TextView::set_text(const std::string& text)
{
  text_area.set_text(text);
}

void
TextView::set_font(wstdisplay::Font const* font)
{
  text_area.set_font(font);
}

} // namespace wstgui

/* EOF */

