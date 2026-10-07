// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/cut_layoutable.hpp>

#include <logmich/log.hpp>

namespace wstgui {

void
CutLayoutable::pack(std::unique_ptr<ILayoutable> layoutable)
{
  if (!m_lhs) {
    m_lhs = std::move(layoutable);
  } else if (!m_rhs) {
    m_rhs = std::move(layoutable);
  } else {
     throw std::runtime_error("CutLayoutable already filled");
  }
}

void
CutLayoutable::set_geometry(geom::frect const& rect)
{
  if (m_orientation == Orientation::Horizontal)
  {
    if (m_lhs) {
      m_lhs->set_geometry({
          rect.left(),
          rect.top(),
          rect.left() + m_length,
          rect.bottom(),
        });
    }

    if (m_rhs) {
      m_rhs->set_geometry({
          rect.left() + m_length,
          rect.top(),
          rect.right(),
          rect.bottom(),
        });
    }
  }
  else // if (m_orientation == Orientation::Vertical)
  {
    if (m_lhs) {
      m_lhs->set_geometry({
          rect.left(),
          rect.top(),
          rect.right(),
          rect.top() + m_length,
        });
    }

    if (m_rhs) {
      m_rhs->set_geometry({
          rect.left(),
          rect.top() + m_length,
          rect.right(),
          rect.bottom(),
        });
    }
  }
}

} // namespace wstgui

/* EOF */
