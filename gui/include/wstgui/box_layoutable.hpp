// SPDX-FileCopyrightText: 2021 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTGUI_BOX_LAYOUTABLE_HPP
#define HEADER_WSTGUI_BOX_LAYOUTABLE_HPP

#include <memory>
#include <vector>

#include "ilayoutable.hpp"

namespace wstgui {

class BoxLayoutable : public ILayoutable
{
public:
  enum class Orientation { Horizontal, Vertical };

public:
  BoxLayoutable(Orientation orientation);

  void pack(std::unique_ptr<ILayoutable> layoutable, bool expand = true, bool fill = true, float margin = 0.0f);
  void pack(Component* component, bool expand = true, bool fill = true, float margin = 0.0f);
  void pack_spacer(float length);
  void pack_stretcher();

  void set_geometry(geom::frect const& rect) override;

private:
  struct Item {
    std::unique_ptr<ILayoutable> layoutable;
    bool expand = false;
    bool fill = true;
    float margin = 0.0f;
  };

private:
  Orientation m_orientation;
  std::vector<Item> m_items;

private:
  BoxLayoutable(const BoxLayoutable&) = delete;
  BoxLayoutable& operator=(const BoxLayoutable&) = delete;
};

class HBox : public BoxLayoutable
{
public:
  HBox() :
    BoxLayoutable(Orientation::Horizontal)
  {}
};

class VBox : public BoxLayoutable
{
public:
  VBox() :
    BoxLayoutable(Orientation::Vertical)
  {}
};

} // namespace wstgui

#endif

/* EOF */
