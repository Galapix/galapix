// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_STYLE_HPP
#define HEADER_WINDSTILLE_STYLE_HPP

#include <wstdisplay/font/font.hpp>

namespace wstgui {

class Style
{
public:
  Style(wstdisplay::Font const* font);
  ~Style();

  wstdisplay::Font const* get_font() const;
  wstdisplay::Font const* get_small_font() const;

private:
  wstdisplay::Font const* m_font;

private:
  Style(const Style&) = delete;
  Style& operator=(const Style&) = delete;
};

} // namespace wstgui

#endif

/* EOF */
