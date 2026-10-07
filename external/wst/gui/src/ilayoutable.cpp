// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/ilayoutable.hpp>

#include <wstgui/component.hpp>

namespace wstgui {

LayoutableComponent::LayoutableComponent(Component* component) :
  m_component(component)
{
}

void
LayoutableComponent::set_geometry(geom::frect const& rect)
{
  m_component->set_geometry(rect);
}

} // namespace wstgui

/* EOF */
