// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/list_view.hpp>

#include <wstinput/controller.hpp>
#include <wstdisplay/canvas.hpp>

#include <wstgui/controller_def.hpp>
#include <wstgui/style.hpp>

namespace wstgui {

ListView::ListView(Component* parent_)
  : Component(parent_),
    columns(),
    items(),
    current_item(0)
{
}

ListView::~ListView()
{
}

void
ListView::draw(wstdisplay::Canvas& canvas)
{
  wstdisplay::Font const* font = get_style().get_font();

  float x = m_geometry.left();
  float y = m_geometry.top() + static_cast<float>(font->get_height());
  float padding = 10;

  for(int i = 0; i < int(columns.size()); ++i)
  {
    // FIXME: Poor mans outline effect
    font->draw(canvas, glm::vec2(x + columns[i].width/2 + 1, y - 1), columns[i].title, surf::Color(1.0f, 1.0f, 1.0f), wstdisplay::TextAlign::Center);
    font->draw(canvas, glm::vec2(x + columns[i].width/2 - 1, y - 1), columns[i].title, surf::Color(1.0f, 1.0f, 1.0f), wstdisplay::TextAlign::Center);
    font->draw(canvas, glm::vec2(x + columns[i].width/2 + 1, y + 1), columns[i].title, surf::Color(1.0f, 1.0f, 1.0f), wstdisplay::TextAlign::Center);
    font->draw(canvas, glm::vec2(x + columns[i].width/2 - 1, y + 1), columns[i].title, surf::Color(1.0f, 1.0f, 1.0f), wstdisplay::TextAlign::Center);
    font->draw(canvas, glm::vec2(x + columns[i].width/2, y), columns[i].title, surf::Color(0.0f, 0.0f, 0.0f), wstdisplay::TextAlign::Center);
    x += columns[i].width;
  }

  for(int j = 0; j < int(items.size()); ++j)
  {
    x = m_geometry.left();

    if (j == current_item)
      canvas.fill_rect(geom::frect(x, y,
                               m_geometry.right(), y + static_cast<float>(font->get_height())),
                         is_active() ? surf::Color(0.5f, 0.5f, 1.0f, 0.8f) : surf::Color(0.5f, 0.5f, 1.0f, 0.3f));

    y += static_cast<float>(font->get_height());

    for(int i = 0; i < int(items[j].columns.size()) && i < int(columns.size()); ++i)
    {
      font->draw(canvas, glm::vec2(x + padding, y), items[j].columns[i]);

      x += columns[i].width;
    }
  }
}

void
ListView::update(float delta, const Controller& controller)
{
  for(auto i = controller.get_events().begin();
      i != controller.get_events().end(); ++i)
  {
    if (i->type == wstinput::BUTTON_EVENT && i->button.down)
    {
      if (i->button.name == OK_BUTTON)
      {
        // do something with item (scripting callback)
      }
      else if (i->button.name == CANCEL_BUTTON || i->button.name == ESCAPE_BUTTON)
      {
        set_active(false);
      }
    }
    else if (i->type == wstinput::AXIS_EVENT)
    {
      if (i->axis.name == Y_AXIS)
      {
        if (i->axis.pos > 0)
        {
          if (current_item == int(items.size()) - 1)
            current_item = 0;
          else
            current_item += 1;
        }
        else if (i->axis.pos < 0)
        {
          if (current_item == 0)
            current_item = static_cast<int>(items.size()) - 1;
          else
            current_item -= 1;
        }
      }
    }
  }
}

void
ListView::add_column(const std::string& title, float width)
{
  Column column;

  column.title = title;
  if (width == -1)
    column.width = 150;
  else
    column.width = width;

  columns.push_back(column);
}

void
ListView::add_item(const Item& item)
{
  items.push_back(item);
}

} // namespace wstgui

/* EOF */
