// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_FONT_BITMAP_FONT_HPP
#define HEADER_WSTDISPLAY_FONT_BITMAP_FONT_HPP

#include <memory>
#include <span>
#include <string_view>
#include <unordered_map>

#include <geom/rect.hpp>
#include <surf/software_surface.hpp>

#include "../device.hpp"
#include "../texture_params.hpp"
#include "font.hpp"

namespace wstdisplay {

/** A font whose glyphs are rectangles on an image, as used by pixel
    art games. The top of a glyph rect is placed \a height pixels
    above the baseline, shifted by its offset. */
class BitmapFont final : public Font
{
public:
  struct GlyphDef
  {
    char32_t codepoint;

    /** Pixel rectangle on the image */
    geom::irect rect;

    geom::ipoint offset = {};

    /** Pen advance, a negative value uses the rect width */
    float advance = -1.0f;
  };

public:
  /** Glyphs on a grid of \a cell sized cells, \a charset lists the
      characters (UTF-8) row by row. With \a proportional the
      transparent columns on both sides of a glyph are trimmed. */
  static std::unique_ptr<BitmapFont> from_grid(Device& device, surf::SoftwareSurface const& image,
                                               geom::isize const& cell,
                                               std::string_view charset, bool proportional = false,
                                               float char_spacing = 0.0f,
                                               TextureParams const& params = {TextureFilter::Nearest});

public:
  /** \a glyphs are rectangles on \a texture, which the font owns */
  BitmapFont(Unique<Texture> texture, std::span<GlyphDef const> glyphs, float height,
             float line_height = 0.0f, float char_spacing = 0.0f);
  ~BitmapFont() override;

  Glyph const* get_glyph(char32_t codepoint) const override;
  float get_height() const override { return m_height; }
  float get_line_height() const override { return m_line_height; }

private:
  Unique<Texture> m_texture;
  std::unordered_map<char32_t, Glyph> m_glyphs;
  float m_height;
  float m_line_height;
};

} // namespace wstdisplay

#endif

/* EOF */
