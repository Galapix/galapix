// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_GUI_COMPONENT_HPP
#define HEADER_WINDSTILLE_GUI_COMPONENT_HPP

#include <geom/geom.hpp>
#include <wstdisplay/fwd.hpp>
#include <wstinput/fwd.hpp>

#include "style.hpp"

namespace wstgui {

using Controller = wstinput::Controller;

class Component
{
public:
  Component(Component* parent);
  virtual ~Component();

  virtual void set_parent(Component* parent);

  virtual bool is_active() const;
  virtual void set_active(bool a);

  /** Called when the Component receives focus */
  virtual void on_activation() {}

  virtual void draw(wstdisplay::Canvas& canvas) =0;
  virtual void update(float delta, const Controller& controller) =0;

  virtual geom::frect geometry() const;
  virtual void  set_geometry(const geom::frect& rect);

  virtual geom::fsize get_prefered_size() const;

  virtual Style& get_style() const;

protected:
  Component* m_parent;

  /** Rectangle on the screen allocated for the Component */
  geom::frect m_geometry;

  /** Size prefered by the widget */
  geom::fsize m_prefered_size;

  bool m_active;

private:
  Component (const Component&);
  Component& operator= (const Component&);
};

} // namespace wstgui

#endif

/* EOF */
