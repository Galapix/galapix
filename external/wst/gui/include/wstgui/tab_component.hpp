// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_TAB_COMPONENT_HPP
#define HEADER_WINDSTILLE_GUI_TAB_COMPONENT_HPP

#include "component.hpp"

namespace wstgui {

/** */
class TabComponent : public Component
{
private:
  struct Tab
  {
    std::string label;
    Component*  component;

    Tab()
      : label(),
        component(nullptr)
    {}

    Tab(const std::string& label_, Component* c)
      : label(label_),
        component(c)
    {}

    Tab(const Tab& rhs)
      : label(rhs.label),
        component(rhs.component)
    {}

    Tab& operator=(const Tab& rhs)
    {
      if (this != &rhs)
      {
        label     = rhs.label;
        component = rhs.component;
      }
      return *this;
    }
  };

  typedef std::vector<Tab> Tabs;
  Tabs tabs;

  int current_tab;

public:
  TabComponent(Component* parent);
  ~TabComponent() override;

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

  void pack(const std::string& name, Component* component);

private:
  TabComponent (const TabComponent&);
  TabComponent& operator= (const TabComponent&);
};

} // namespace wstgui

#endif

/* EOF */
