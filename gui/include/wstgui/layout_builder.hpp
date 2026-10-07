// SPDX-FileCopyrightText: 2021 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTGUI_LAYOUT_BUILDER_HPP
#define HEADER_WSTGUI_LAYOUT_BUILDER_HPP

#include <memory>
#include <vector>

#include "ilayoutable.hpp"
#include "cut_layoutable.hpp"

namespace wstgui {

class Component;

class LayoutBuilder
{
public:
  LayoutBuilder();

  LayoutBuilder& begin_hcut(float length);
  LayoutBuilder& begin_vcut(float length);
  LayoutBuilder& next();
  LayoutBuilder& pack(Component* component);
  LayoutBuilder& pack(std::unique_ptr<ILayoutable> layoutable);
  LayoutBuilder& end();

  std::unique_ptr<ILayoutable> finalize();

private:
  std::vector<std::unique_ptr<CutLayoutable>> m_stack;
  std::unique_ptr<ILayoutable> m_root;

private:
  LayoutBuilder(const LayoutBuilder&) = delete;
  LayoutBuilder& operator=(const LayoutBuilder&) = delete;
};

} // namespace wstgui

#endif

/* EOF */
