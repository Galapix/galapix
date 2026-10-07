// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_GUI_MANAGER_HPP
#define HEADER_WINDSTILLE_GUI_GUI_MANAGER_HPP

#include <memory>

#include <wstdisplay/canvas.hpp>
#include <wstinput/controller.hpp>

#include "fwd.hpp"
#include "screen.hpp"

namespace wstgui {

class GUIManager : public Screen
{
public:
  GUIManager(Style& style);
  ~GUIManager() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const wstinput::Controller& controller) override;

  RootComponent* get_root() const;
  Style& get_style() const { return m_style; }

private:
  std::unique_ptr<RootComponent> m_root;
  Style& m_style;
  Component* m_focus_component;

private:
  GUIManager (const GUIManager&);
  GUIManager& operator= (const GUIManager&);
};

} // namespace wstgui

#endif

/* EOF */
