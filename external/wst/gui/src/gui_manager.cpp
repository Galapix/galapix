// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/gui_manager.hpp>

#include <logmich/log.hpp>

#include <wstgui/root_component.hpp>

namespace wstgui {

GUIManager::GUIManager(Style& style) :
  m_root(new RootComponent(style)),
  m_style(style),
  m_focus_component()
{
}

GUIManager::~GUIManager()
{
}

void
GUIManager::draw(wstdisplay::Canvas& canvas)
{
  m_root->draw(canvas);

  if (m_focus_component) {
    canvas.fill_rect(m_focus_component->geometry(), surf::Color(1.0f, 0.0f, 0.0f, 0.5f));
  }
}

void
GUIManager::update(float delta, const Controller& controller)
{
  m_root->update(delta, controller);

  geom::fpoint mouse_pos(controller.get_pointer_state(0), controller.get_pointer_state(1));
  m_focus_component = m_root->query(mouse_pos);

  if (!m_root->is_active()) {
    finish();
  }
}

RootComponent*
GUIManager::get_root() const
{
  return m_root.get();
}

} // namespace wstgui

/* EOF */
