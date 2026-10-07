// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/tab_component.hpp>

#include <wstinput/controller.hpp>

#include <logmich/log.hpp>
#include <wstdisplay/canvas.hpp>

#include <wstgui/controller_def.hpp>

namespace wstgui {

TabComponent::TabComponent(Component* parent_)
  : Component(parent_),
    tabs(),
    current_tab(0)
{
}

TabComponent::~TabComponent()
{
}

void
TabComponent::draw(wstdisplay::Canvas& canvas)
{
  if (tabs.empty()) return;

  float tab_width = m_geometry.width() / static_cast<float>(tabs.size());
  for(int i = 0; i != int(tabs.size()); ++i)
  {
    geom::frect tab_rect(glm::vec2(m_geometry.left() + tab_width * static_cast<float>(i) + 10.0f,
                                   m_geometry.top()),
                         geom::fsize(tab_width - 20.0f, static_cast<float>(get_style().get_font()->get_height()) + 6.0f));

    if (i == current_tab)
      canvas.fill_rounded_rect(tab_rect, 5.0f, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));
    else
      canvas.fill_rounded_rect(tab_rect, 5.0f, surf::Color(0.0f, 0.0f, 0.0f, 0.5f));

    canvas.draw_rounded_rect(tab_rect, 5.0f, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

    get_style().get_font()->draw(canvas,
                                      glm::vec2(m_geometry.left() + tab_width * static_cast<float>(i) + tab_width/2,
                                                m_geometry.top() + static_cast<float>(get_style().get_font()->get_height())),
                                      tabs[i].label,
                                      tabs[current_tab].component->is_active()
                                      ? surf::Color(1.0f, 1.0f, 1.0f, 0.5f)
                                      : surf::Color(1.0f, 1.0f, 1.0f, 1.0f), wstdisplay::TextAlign::Center);
  }

  tabs[current_tab].component->draw(canvas);
}

void
TabComponent::update(float delta, const Controller& controller)
{
  if (tabs.empty()) return;

  if (tabs[current_tab].component->is_active())
  {
    tabs[current_tab].component->update(delta, controller);
  }
  else
  {
    tabs[current_tab].component->update(delta, Controller());

    for(auto i = controller.get_events().begin(); i != controller.get_events().end(); ++i)
    {
      if (i->type == wstinput::BUTTON_EVENT && i->button.down)
      {
        if (i->button.name == OK_BUTTON || i->button.name == ENTER_BUTTON)
        {
          tabs[current_tab].component->set_active(true);
        }
        else if (i->button.name == CANCEL_BUTTON || i->button.name == ESCAPE_BUTTON)
        {
          tabs[current_tab].component->set_active(false);
          set_active(false);
        }
      }
      else if (i->type == wstinput::AXIS_EVENT)
      {
        if (i->axis.name == X_AXIS)
        {
          if (i->axis.pos < 0)
          {
            if (current_tab == 0)
              current_tab = static_cast<int>(tabs.size()) - 1;
            else
              current_tab -= 1;
          }
          else if (i->axis.pos > 0)
          {
            if (current_tab == int(tabs.size()) - 1)
              current_tab = 0;
            else
              current_tab += 1;
          }
        }
        else if (i->axis.name == Y_AXIS)
        {
          if (i->axis.pos < 0)
          {
          }
          else if (i->axis.pos > 0)
          {
            tabs[current_tab].component->set_active(true);
          }
        }
      }
    }
  }
}

void
TabComponent::pack(const std::string& name, Component* component)
{
  tabs.push_back(Tab(name, component));

  float padding = 6.0f;
  component->set_geometry(geom::frect(m_geometry.left() + padding,
                                   m_geometry.top()  + padding + static_cast<float>(get_style().get_font()->get_height()) + 10.0f,
                                   m_geometry.right()  - padding,
                                   m_geometry.bottom() - padding
                               ));
}

} // namespace wstgui

/* EOF */
