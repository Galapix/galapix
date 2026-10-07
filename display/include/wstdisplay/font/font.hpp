// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FONT_FONT_HPP
#define HEADER_WSTDISPLAY_FONT_FONT_HPP

#include <string_view>

#include <geom/point.hpp>
#include <geom/rect.hpp>
#include <surf/color.hpp>

#include "../canvas.hpp"
#include "../fwd.hpp"

namespace wstdisplay {

struct Glyph
{
  /** Null for glyphs without an image, e.g. space */
  TextureId texture = {};

  /** Texture coordinates of the glyph image */
  geom::frect uv = {};

  /** Position of the glyph image relative to the pen position on the
      baseline, top is negative for glyphs above the baseline */
  geom::frect rect = {};

  /** Horizontal pen movement after this glyph */
  float advance = 0.0f;
};

/** Decode the UTF-8 codepoint at \a pos and advance \a pos past it,
    invalid sequences decode as U+FFFD */
char32_t utf8_next(std::string_view text, size_t& pos);

/** Base class for fonts, implements layout and drawing on top of
    get_glyph(). Text is UTF-8, '\n' starts a new line. */
class Font
{
public:
  virtual ~Font();

  /** The glyph for \a codepoint, nullptr if the font doesn't have it */
  virtual Glyph const* get_glyph(char32_t codepoint) const = 0;

  /** Nominal size of the font in pixels */
  virtual float get_height() const = 0;

  /** Distance between baselines of consecutive lines */
  virtual float get_line_height() const = 0;

  /** The glyph for \a codepoint, or a replacement glyph ('?') */
  Glyph const* get_glyph_or_fallback(char32_t codepoint) const;

  /** Width of the widest line */
  float get_width(std::string_view text) const;

  /** Size of the text block, height is lines * line height */
  geom::fsize get_size(std::string_view text) const;

  /** Draw \a text, \a pos is the left/center/right end of the
      baseline of the first line, depending on \a align */
  void draw(Canvas& canvas, geom::fpoint const& pos, std::string_view text,
            surf::Color const& color = {1.0f, 1.0f, 1.0f}, TextAlign align = TextAlign::Left) const;

  /** Draw a single glyph scaled by \a scale with a vertical color gradient */
  static void draw_glyph(Canvas& canvas, Glyph const& glyph, geom::fpoint const& pen, float scale,
                         surf::Color const& top, surf::Color const& bottom);
};

} // namespace wstdisplay

#endif

/* EOF */
