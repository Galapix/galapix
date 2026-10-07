// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTGUI_CUT_LAYOUTABLE_HPP
#define HEADER_WSTGUI_CUT_LAYOUTABLE_HPP

#include <memory>

#include "ilayoutable.hpp"

namespace wstgui {

class CutLayoutable : public ILayoutable
{
public:
  enum class Orientation { Horizontal, Vertical };

public:
  CutLayoutable(Orientation orientation, float length) :
    m_orientation(orientation),
    m_length(length),
    m_lhs(),
    m_rhs()
  {}

  void pack(std::unique_ptr<ILayoutable> layoutable);

  void set_geometry(geom::frect const& rect) override;

private:
  Orientation m_orientation;
  float m_length;
  std::unique_ptr<ILayoutable> m_lhs;
  std::unique_ptr<ILayoutable> m_rhs;
};

} // namespace wstgui

#endif

/* EOF */
