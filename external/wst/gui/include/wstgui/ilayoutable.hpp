// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTGUI_LAYOUTABLE_HPP
#define HEADER_WSTGUI_LAYOUTABLE_HPP

#include <optional>

#include <geom/rect.hpp>

namespace wstgui {

class Component;

class ILayoutable
{
public:
  virtual ~ILayoutable() {}

  virtual void set_geometry(geom::frect const& rect) {}
  virtual std::optional<geom::fsize> get_size_hint() const { return std::nullopt; }
};

class SpacerLayoutable : public ILayoutable
{
public:
  SpacerLayoutable(geom::fsize const& size_hint) :
    m_size_hint(size_hint)
  {}

  std::optional<geom::fsize> get_size_hint() const override { return m_size_hint; }

private:
  geom::fsize m_size_hint;
};

class LayoutableComponent : public ILayoutable
{
public:
  LayoutableComponent(Component* component);
  virtual ~LayoutableComponent() {}

  void set_geometry(geom::frect const& rect) override;

private:
  Component* m_component;

public:
  LayoutableComponent(const LayoutableComponent&) = delete;
  LayoutableComponent& operator=(const LayoutableComponent&) = delete;
};

} // namespace wstgui

#endif

/* EOF */
