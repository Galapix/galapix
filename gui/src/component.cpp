// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/component.hpp>

namespace wstgui {

Component::Component(Component* parent) :
  m_parent(parent),
  m_geometry(),
  m_prefered_size(),
  m_active(false)
{
}

Component::~Component()
{
}

void
Component::set_parent(Component* parent)
{
  m_parent = parent;
}

bool
Component::is_active() const
{
  return m_active;
}

void
Component::set_active(bool active)
{
  if (!m_active && active) {
    on_activation();
  }
  m_active = active;
}

geom::frect
Component::geometry() const
{
  return m_geometry;
}

void
Component::set_geometry(const geom::frect& rect)
{
  m_geometry = rect;
}

geom::fsize
Component::get_prefered_size() const
{
  return m_prefered_size;
}

Style&
Component::get_style() const
{
  return m_parent->get_style();
}

} // namespace wstgui

/* EOF */
