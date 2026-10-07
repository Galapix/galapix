// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/style.hpp>

namespace wstgui {

Style::Style(wstdisplay::Font const* font) :
  m_font(font)
{
}

Style::~Style()
{
}

wstdisplay::Font const*
Style::get_font() const
{
  return m_font;
}

wstdisplay::Font const*
Style::get_small_font() const
{
  return m_font;
}

} // namespace wstgui

/* EOF */
