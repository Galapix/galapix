// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/button.hpp>

#include <logmich/log.hpp>
#include <wstinput/controller.hpp>
#include <wstdisplay/canvas.hpp>

#include <wstgui/controller_def.hpp>
#include <wstgui/tab_component.hpp>

namespace wstgui {

Button::Button(const std::string& label_, Component* parent_) :
  Component(parent_),
  label(label_)
{
}

Button::~Button()
{
}

void
Button::draw(wstdisplay::Canvas& canvas)
{
  canvas.fill_rect(m_geometry, surf::Color(0.0f, 0.0f, 0.0f, 0.5f));
  canvas.draw_rect(m_geometry, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));
  get_style().get_font()->draw(
    canvas,
    glm::vec2(m_geometry.left() + m_geometry.width()/2.0f,
              m_geometry.top() + m_geometry.height()/2.0f + static_cast<float>(get_style().get_font()->get_height()) / 3.0f),
    label,
    is_active()
    ? surf::Color(1.0f, 1.0f, 1.0f, 1.0f)
    : surf::Color(1.0f, 1.0f, 1.0f, 0.5f), wstdisplay::TextAlign::Center);
}

void
Button::update(float delta, const Controller& controller)
{
  set_active(false);

  for(auto i = controller.get_events().begin(); i != controller.get_events().end(); ++i)
  {
    if (i->type == wstinput::BUTTON_EVENT && i->button.down)
    {
      if (i->button.name == OK_BUTTON)
      {
      }
      else if (i->button.name == CANCEL_BUTTON || i->button.name == ESCAPE_BUTTON)
      {
        set_active(false);
      }
    }
  }
}

} // namespace wstgui

/* EOF */
