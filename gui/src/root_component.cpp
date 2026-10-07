// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>

#include <logmich/log.hpp>
#include <wstinput/controller.hpp>

#include <wstgui/root_component.hpp>

namespace wstgui {

RootComponent::RootComponent(Style& style) :
  Component(nullptr),
  m_focus(nullptr),
  m_children(),
  m_style(style)
{
  set_active(true);
}

RootComponent::~RootComponent()
{
}

void
RootComponent::draw(wstdisplay::Canvas& canvas)
{
  for (auto i = m_children.begin(); i != m_children.end(); ++i)
  {
    (*i)->draw(canvas);
  }
}

void
RootComponent::update(float delta, const Controller& controller)
{
  for (auto i = m_children.begin(); i != m_children.end(); ++i)
  {
    if (i->get() == m_focus)
      (*i)->update(delta, controller);
    else
      (*i)->update(delta, Controller());
  }
}

bool
RootComponent::is_active() const
{
  // cancelling the focused component, e.g. a menu, closes the GUI;
  // without focus there is nothing that could be cancelled
  if (m_focus) {
    return m_focus->is_active();
  }
  return true;
}

void
RootComponent::add_child(std::unique_ptr<Component> child)
{
  // the most recently added component that takes input gets the
  // focus, e.g. a menu, but not a label added after it
  if (child->is_active()) {
    m_focus = child.get();
  }
  m_children.emplace_back(std::move(child));
}

void
RootComponent::set_focus(Component* child_)
{
  auto const i = std::find_if(m_children.begin(), m_children.end(),
                              [child_](std::unique_ptr<Component> const& it){ return it.get() == child_; });
  if (i != m_children.end())
  {
    m_focus = child_;
    m_focus->set_active(true);
  }
  else
  {
    log_error("Error: Need to add_child() first befor calling set_m_focus()");
  }
}

Style&
RootComponent::get_style() const
{
  return m_style;
}

Component*
RootComponent::query(geom::fpoint const& pos) const
{
  for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
    auto const& component = *it;

    geom::frect const rect = component->geometry();
    if (geom::contains(rect, pos)) {
      return component.get();
    }
  }

  return nullptr;
}

} // namespace wstgui

/* EOF */
