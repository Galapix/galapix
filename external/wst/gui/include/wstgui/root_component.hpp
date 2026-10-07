// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_ROOT_COMPONENT_HPP
#define HEADER_WINDSTILLE_GUI_ROOT_COMPONENT_HPP

#include <memory>
#include <vector>

#include "component.hpp"
#include "style.hpp"

namespace wstgui {

class RootComponent : public Component
{
public:
  RootComponent(Style& style);
  ~RootComponent() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

  /** Set the chidren that shall recieve input */
  /** Give \a child, which must have been added, the input focus */
  void set_focus(Component* child);

  /** Add \a child, it gets the focus if it is active */
  void add_child(std::unique_ptr<Component> child);

  Component* get_focus() const { return m_focus; }

  Component* query(geom::fpoint const& pos) const;

  /** False once the focused component was deactivated, e.g. a menu
      was cancelled, which closes the GUIManager */
  bool is_active() const override;

  Style& get_style() const override;

private:
  Component* m_focus;
  std::vector<std::unique_ptr<Component>> m_children;
  Style& m_style;

private:
  RootComponent (const RootComponent&) = delete;
  RootComponent& operator= (const RootComponent&) = delete;
};

} // namespace wstgui

#endif

/* EOF */
