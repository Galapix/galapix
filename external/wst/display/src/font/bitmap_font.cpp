// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/font/bitmap_font.hpp>

#include <vector>

#include <wstdisplay/texture.hpp>

namespace wstdisplay {

std::unique_ptr<BitmapFont>
BitmapFont::from_grid(Device& device, surf::SoftwareSurface const& image, geom::isize const& cell,
                      std::string_view charset, bool proportional, float char_spacing,
                      TextureParams const& params)
{
  int const columns = image.get_width() / cell.width();
  std::vector<GlyphDef> defs;

  int index = 0;
  for (size_t pos = 0; pos < charset.size(); ++index) {
    char32_t const cp = utf8_next(charset, pos);
    int const cx = (index % columns) * cell.width();
    int const cy = (index / columns) * cell.height();
    if (cy + cell.height() > image.get_height()) {
      break;
    }

    geom::irect rect(geom::ipoint(cx, cy), cell);
    float advance = -1.0f;

    if (proportional) {
      auto const column_empty = [&](int x) {
        for (int y = cy; y < cy + cell.height(); ++y) {
          if (image.get_pixel(geom::ipoint(x, y)).a > 0.0f) {
            return false;
          }
        }
        return true;
      };

      int left = cx;
      int right = cx + cell.width();
      while (left < right && column_empty(left)) { ++left; }
      while (right > left && column_empty(right - 1)) { --right; }

      if (left == right) {
        // empty cell, e.g. space
        advance = static_cast<float>(cell.width()) / 2.0f;
        rect = geom::irect(geom::ipoint(cx, cy), geom::isize(0, 0));
      } else {
        rect = geom::irect(left, cy, right, cy + cell.height());
      }
    }

    defs.push_back(GlyphDef{cp, rect, {}, advance});
  }

  return std::make_unique<BitmapFont>(device.create_texture(image, params), defs,
                                      static_cast<float>(cell.height()),
                                      static_cast<float>(cell.height()), char_spacing);
}

BitmapFont::BitmapFont(Unique<Texture> texture, std::span<GlyphDef const> glyphs, float height,
                       float line_height, float char_spacing) :
  m_texture(std::move(texture)),
  m_glyphs(),
  m_height(height),
  m_line_height(line_height > 0.0f ? line_height : height)
{
  float const tw = static_cast<float>(m_texture->get_width());
  float const th = static_cast<float>(m_texture->get_height());

  for (auto const& def : glyphs) {
    geom::frect const uv(static_cast<float>(def.rect.left()) / tw,
                         static_cast<float>(def.rect.top()) / th,
                         static_cast<float>(def.rect.right()) / tw,
                         static_cast<float>(def.rect.bottom()) / th);

    float const x = static_cast<float>(def.offset.x());
    float const y = static_cast<float>(def.offset.y()) - height;
    geom::frect const rect(x, y,
                           x + static_cast<float>(def.rect.width()),
                           y + static_cast<float>(def.rect.height()));

    float const advance = (def.advance < 0.0f ? static_cast<float>(def.rect.width()) : def.advance) + char_spacing;
    bool const empty = def.rect.width() <= 0 || def.rect.height() <= 0;

    m_glyphs[def.codepoint] = Glyph{empty ? TextureId() : m_texture.get(), uv, rect, advance};
  }
}

BitmapFont::~BitmapFont()
{
}

Glyph const*
BitmapFont::get_glyph(char32_t codepoint) const
{
  auto it = m_glyphs.find(codepoint);
  return it != m_glyphs.end() ? &it->second : nullptr;
}

} // namespace wstdisplay

/* EOF */
