// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FONT_FONT_EFFECT_HPP
#define HEADER_WSTDISPLAY_FONT_FONT_EFFECT_HPP

#include <compare>

namespace wstdisplay {

/** How the glyphs of a TTF font are rasterized */
struct FontEffect
{
  /** Width of a border around the glyphs in pixels, 0 for none. The
      font height and the glyphs grow by twice the border. */
  int border = 0;

  /** true draws a black border around the white glyphs, false makes
      the border white too, which gives a bold look */
  bool outline = true;

  friend auto operator<=>(FontEffect const& lhs, FontEffect const& rhs) = default;
};

} // namespace wstdisplay

#endif

/* EOF */
