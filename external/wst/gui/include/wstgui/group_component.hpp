// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_GROUP_COMPONENT_HPP
#define HEADER_WINDSTILLE_GUI_GROUP_COMPONENT_HPP

#include <memory>

#include "component.hpp"

namespace wstgui {

class GroupComponent : public Component
{
public:
  GroupComponent(const std::string& title, Component* parent);
  ~GroupComponent() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;
  void set_geometry(geom::frect const& rect) override;

  void pack(std::unique_ptr<Component> component);

  bool is_active() const override;

  geom::frect get_child_rect() const;

  bool has_title() const { return !m_title.empty(); }

private:
  std::string m_title;
  std::unique_ptr<Component> m_child;

private:
  GroupComponent (const GroupComponent&);
  GroupComponent& operator= (const GroupComponent&);
};

} // namespace wstgui

#endif

/* EOF */
