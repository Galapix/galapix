// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstdisplay/canvas.hpp>

#include <wstgui/group_component.hpp>
#include <wstgui/style.hpp>

namespace wstgui {

GroupComponent::GroupComponent(const std::string& title, Component* parent) :
  Component(parent),
  m_title(title),
  m_child()
{
}

GroupComponent::~GroupComponent()
{
}

void
GroupComponent::draw(wstdisplay::Canvas& canvas)
{
  canvas.fill_rounded_rect(m_geometry, 5.0f, surf::Color(0.0f, 0.0f, 0.0f, 0.7f));
  canvas.draw_rounded_rect(m_geometry, 5.0f, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

  if (!m_title.empty())
  {
    wstdisplay::Font const* font = get_style().get_font();
    font->draw(canvas,
                      glm::vec2(m_geometry.left() + m_geometry.width() / 2.0f,
                                m_geometry.top()  + static_cast<float>(font->get_height()) + 5.0f),
                      m_title, surf::Color(1.0f, 1.0f, 1.0f), wstdisplay::TextAlign::Center);

    canvas.fill_rect(geom::frect(m_geometry.left()  + 8.0f, m_geometry.top() + static_cast<float>(font->get_height()) + 16.0f,
                             m_geometry.right() - 8.0f, m_geometry.top() + static_cast<float>(font->get_height()) + 18.0f),
                 surf::Color(1.0f, 1.0f, 1.0f, 0.5f));
  }

  if (m_child) {
    m_child->draw(canvas);
  }
}

void
GroupComponent::update(float delta, const Controller& controller)
{
  if (m_child) {
    m_child->update(delta, controller);
  }
}

void
GroupComponent::pack(std::unique_ptr<Component> component)
{
  m_child = std::move(component);
  m_child->set_active(true);
}

geom::frect
GroupComponent::get_child_rect() const
{
  float const padding = 6.0f;

  return geom::frect(m_geometry.left() + padding,
                     m_geometry.top() + padding + (m_title.empty() ?
                                                   0.0f :
                                                   static_cast<float>(get_style().get_font()->get_height()) + 18.0f),
                     m_geometry.right()  - padding,
                     m_geometry.bottom() - padding);
}

void
GroupComponent::set_geometry(geom::frect const& rect)
{
  Component::set_geometry(rect);
  if (m_child) {
    m_child->set_geometry(get_child_rect());
  }
}

bool
GroupComponent::is_active() const
{
  if (m_child) {
    return m_child->is_active();
  } else {
    return false;
  }
}

} // namespace wstgui

/* EOF */
