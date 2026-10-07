// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/menu_item.hpp>

#include <wstdisplay/canvas.hpp>

#include <wstgui/menu_component.hpp>
#include <wstgui/style.hpp>

namespace wstgui {

MenuItem::MenuItem(MenuComponent* parent_, const std::string& label_)
  : parent(parent_),
    label(label_),
    fade_timer(0.0f)
{}

void
MenuItem::draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active)
{
  surf::Color font_color;
  wstdisplay::Font const* font = parent->get_font();

  if (is_active)
  {
    canvas.fill_rounded_rect(rect, 5.0f, surf::Color(0.5f, 0.5f, 0.5f, 0.75f));
    canvas.draw_rounded_rect(rect, 5.0f, surf::Color(1.0f, 1.0f, 1.0f, 1.0f));
    font_color = surf::Color(1.0f, 1.0f, 1.0f);
    fade_timer = 2.0f;
  }
  else
  {
    if (fade_timer != 0.0f)
    {
      //canvas.fill_rounded_rect(rect, 5.0f, surf::Color(0.5f, 0.5f, 0.5f, 0.75f * fade_timer));
      //canvas.draw_rounded_rect(rect, 5.0f, surf::Color(1.0f, 1.0f, 1.0f, 1.0f * fade_timer));
      font_color = surf::Color(0.75f + 0.25f * std::clamp(fade_timer, 0.0f, 1.0f),
                         0.75f + 0.25f * std::clamp(fade_timer, 0.0f, 1.0f),
                         0.75f + 0.25f * std::clamp(fade_timer, 0.0f, 1.0f),
                         1.0f);
    }
    else
    {
      font_color = surf::Color(0.75f, 0.75f, 0.75f, 1.0f);
    }
  }

  font->draw(canvas, glm::vec2(rect.left() + static_cast<float>(font->get_height()),
                           rect.top() + static_cast<float>(font->get_height())/2.0f + rect.height() / 2.0f - 2.0f),
             label, font_color);
}

void
MenuItem::update(float delta)
{
  if (fade_timer > 0.0f)
    fade_timer -= delta / 0.25f;
  else
    fade_timer = 0.0f;
}

EnumMenuItem::EnumMenuItem(MenuComponent* parent_,
                           const std::string& label_, int index_)
  : MenuItem(parent_, label_),
    index(index_),
    labels(),
    on_change()
{
}

EnumMenuItem&
EnumMenuItem::add_pair(int value_, const std::string& label_)
{
  EnumValue enum_value;
  enum_value.value = value_;
  enum_value.label = label_;
  labels.push_back(enum_value);

  return *this;
}

void
EnumMenuItem::incr()
{
  // FIXMESOUND: g_app.sound().play(Pathname("sounds/menu_click.wav", Pathname::kDataPath));

  index -= 1;
  if (index < 0)
    index = static_cast<int>(labels.size()) - 1;
  on_change(labels[index].value);
}

void
EnumMenuItem::decr()
{
  // FIXMESOUND: g_app.sound().play(Pathname("sounds/menu_click.wav", Pathname::kDataPath));

  index += 1;
  if (index >= static_cast<int>(labels.size()))
    index = 0;
  on_change(labels[index].value);
}

void
EnumMenuItem::draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active)
{
  MenuItem::draw(canvas, rect, is_active);
  wstdisplay::Font const* font = parent->get_font();
  surf::Color font_color;
  if (is_active)
  {
    font_color = surf::Color(1.0f, 1.0f, 1.0f);
  }
  else
  {
    font_color = surf::Color(0.75f, 0.75f, 0.75f, 1.0f);
  }

  font->draw(canvas,
             glm::vec2(rect.right() - static_cast<float>(font->get_height())      - static_cast<float>(font->get_width(labels[index].label)),
                       rect.top()   + static_cast<float>(font->get_height())/2.0f + rect.height() / 2.0f - 2.0f),
             labels[index].label,
             font_color);
}

SliderMenuItem::SliderMenuItem(MenuComponent* parent_,
                               const std::string& label_,
                               int value_, int min_value_, int max_value_, int step_)
  : MenuItem(parent_, label_),
    value(value_),
    min_value(min_value_),
    max_value(max_value_),
    step(step_),
    on_change()
{
}

void
SliderMenuItem::decr()
{
  // FIXMESOUND: g_app.sound().play(Pathname("sounds/menu_click.wav", Pathname::kDataPath));

  value += step;
  if (value > max_value)
    value = max_value;
  on_change(value);
}

void
SliderMenuItem::incr()
{
  // FIXMESOUND: g_app.sound().play(Pathname("sounds/menu_click.wav", Pathname::kDataPath));

  value -= step;
  if (value < min_value)
    value = min_value;
  on_change(value);
}

void
SliderMenuItem::draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active)
{
  MenuItem::draw(canvas, rect, is_active);
  float total_width = 200.0f;
  float width = total_width * static_cast<float>(value) / static_cast<float>(max_value - min_value);

  surf::Color color;
  if (is_active)
  {
    color = surf::Color(1.0f, 1.0f, 1.0f);
  }
  else
  {
    color = surf::Color(0.75f, 0.75f, 0.75f, 1.0f);
  }

  canvas.fill_rounded_rect(geom::frect(glm::vec2(rect.right() - 4.0f - total_width, rect.top() + 4.0f),
                                   geom::fsize(width, rect.height() - 8)),
                             5.0f,
                             surf::Color(0.75f*color.r, 0.75f*color.g, 0.75f*color.b, color.a));

  canvas.draw_rounded_rect(geom::frect(glm::vec2(rect.right() - 4.0f - total_width, rect.top() + 4.0f),
                                   geom::fsize(total_width, rect.height() - 8)),
                             5.0f,
                             color);
}

ButtonMenuItem::ButtonMenuItem(MenuComponent* parent_, const std::string& label_)
  : MenuItem(parent_, label_),
    on_click()
{
}

void
ButtonMenuItem::click()
{
  // FIXMESOUND: g_app.sound().play(Pathname("sounds/menu_click.wav", Pathname::kDataPath));

  on_click();
}

void
ButtonMenuItem::draw(wstdisplay::Canvas& canvas, const geom::frect& rect, bool is_active)
{
  MenuItem::draw(canvas, rect, is_active);
}

} // namespace wstgui

/* EOF */
