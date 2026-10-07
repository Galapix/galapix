// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FONT_TTF_FONT_HPP
#define HEADER_WSTDISPLAY_FONT_TTF_FONT_HPP

// Internal header, TTF fonts are created through FontManager

#include <filesystem>
#include <memory>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <wstdisplay/font/font.hpp>
#include <wstdisplay/font/font_effect.hpp>

namespace wstdisplay {

class TTFFontImpl;

/** A TrueType font rendered with FreeType. Glyphs are rasterized on
    first use into atlas pages, Latin-1 is prepared up front. */
class TTFFont final : public Font
{
public:
  TTFFont(Device& device, FT_Library freetype, std::filesystem::path const& file, int size,
          FontEffect const& effect);
  ~TTFFont() override;

  Glyph const* get_glyph(char32_t codepoint) const override;

  /** The size given in the constructor, grown by the border */
  float get_height() const override;
  float get_line_height() const override;

private:
  std::unique_ptr<TTFFontImpl> impl;

public:
  TTFFont(TTFFont const&) = delete;
  TTFFont& operator=(TTFFont const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
