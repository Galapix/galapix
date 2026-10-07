// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/font/font.hpp>

#include <algorithm>
#include <cmath>
#include <vector>


namespace wstdisplay {

char32_t utf8_next(std::string_view text, size_t& pos)
{
  constexpr char32_t replacement = 0xfffd;

  auto const byte = [&](size_t i) { return static_cast<unsigned char>(text[i]); };

  unsigned char const c = byte(pos);
  if (c < 0x80) {
    pos += 1;
    return c;
  }

  int len = 0;
  char32_t cp = 0;
  if ((c & 0xe0) == 0xc0) { len = 2; cp = c & 0x1f; }
  else if ((c & 0xf0) == 0xe0) { len = 3; cp = c & 0x0f; }
  else if ((c & 0xf8) == 0xf0) { len = 4; cp = c & 0x07; }
  else {
    pos += 1;
    return replacement;
  }

  if (pos + static_cast<size_t>(len) > text.size()) {
    pos = text.size();
    return replacement;
  }

  for (int i = 1; i < len; ++i) {
    unsigned char const cc = byte(pos + static_cast<size_t>(i));
    if ((cc & 0xc0) != 0x80) {
      pos += static_cast<size_t>(i);
      return replacement;
    }
    cp = (cp << 6) | (cc & 0x3f);
  }

  pos += static_cast<size_t>(len);
  return cp;
}

Font::~Font()
{
}

Glyph const*
Font::get_glyph_or_fallback(char32_t codepoint) const
{
  if (Glyph const* glyph = get_glyph(codepoint)) {
    return glyph;
  }
  return get_glyph(U'?');
}

float
Font::get_width(std::string_view text) const
{
  float width = 0.0f;
  float line_width = 0.0f;
  for (size_t pos = 0; pos < text.size();) {
    char32_t const cp = utf8_next(text, pos);
    if (cp == U'\n') {
      width = std::max(width, line_width);
      line_width = 0.0f;
    } else if (Glyph const* glyph = get_glyph_or_fallback(cp)) {
      line_width += glyph->advance;
    }
  }
  return std::max(width, line_width);
}

geom::fsize
Font::get_size(std::string_view text) const
{
  int const lines = 1 + static_cast<int>(std::count(text.begin(), text.end(), '\n'));
  return geom::fsize(get_width(text), static_cast<float>(lines) * get_line_height());
}

void
Font::draw(Canvas& canvas, geom::fpoint const& pos, std::string_view text,
           surf::Color const& color, TextAlign align) const
{
  PackedColor const pc = pack_color(color);

  std::vector<Vertex> quads;
  TextureId texture = {};
  auto const flush = [&]() {
    if (!quads.empty()) {
      canvas.draw_quads(texture, quads);
      quads.clear();
    }
  };

  float y = pos.y();
  size_t line_start = 0;
  while (line_start <= text.size())
  {
    size_t line_end = text.find('\n', line_start);
    if (line_end == std::string_view::npos) {
      line_end = text.size();
    }
    std::string_view const line = text.substr(line_start, line_end - line_start);

    float x = pos.x();
    if (align != TextAlign::Left) {
      float const width = get_width(line);
      x -= (align == TextAlign::Center) ? std::floor(width / 2.0f) : width;
    }

    for (size_t i = 0; i < line.size();) {
      Glyph const* glyph = get_glyph_or_fallback(utf8_next(line, i));
      if (!glyph) {
        continue;
      }

      if (glyph->texture && glyph->rect.width() > 0.0f) {
        if (glyph->texture != texture) {
          flush();
          texture = glyph->texture;
        }

        geom::frect const& r = glyph->rect;
        geom::frect const& uv = glyph->uv;
        quads.push_back(Vertex{x + r.left(), y + r.top(), uv.left(), uv.top(), pc});
        quads.push_back(Vertex{x + r.right(), y + r.top(), uv.right(), uv.top(), pc});
        quads.push_back(Vertex{x + r.right(), y + r.bottom(), uv.right(), uv.bottom(), pc});
        quads.push_back(Vertex{x + r.left(), y + r.bottom(), uv.left(), uv.bottom(), pc});
      }
      x += glyph->advance;
    }

    y += get_line_height();
    line_start = line_end + 1;
  }

  flush();
}

void
Font::draw_glyph(Canvas& canvas, Glyph const& glyph, geom::fpoint const& pen, float scale,
                 surf::Color const& top, surf::Color const& bottom)
{
  if (!glyph.texture || glyph.rect.width() <= 0.0f) {
    return;
  }

  geom::frect const& r = glyph.rect;
  geom::frect const& uv = glyph.uv;
  PackedColor const tc = pack_color(top);
  PackedColor const bc = pack_color(bottom);

  Vertex const quad[] = {
    {pen.x() + r.left() * scale, pen.y() + r.top() * scale, uv.left(), uv.top(), tc},
    {pen.x() + r.right() * scale, pen.y() + r.top() * scale, uv.right(), uv.top(), tc},
    {pen.x() + r.right() * scale, pen.y() + r.bottom() * scale, uv.right(), uv.bottom(), bc},
    {pen.x() + r.left() * scale, pen.y() + r.bottom() * scale, uv.left(), uv.bottom(), bc},
  };
  canvas.draw_quads(glyph.texture, quad);
}

} // namespace wstdisplay

/* EOF */
