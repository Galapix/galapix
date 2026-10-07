// SPDX-FileCopyrightText: 2005-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "font/ttf_font.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <logmich/log.hpp>
#include <surf/palette.hpp>
#include <surf/software_surface.hpp>

#include <wstdisplay/blitter.hpp>
#include <wstdisplay/device.hpp>
#include <wstdisplay/rect_packer.hpp>
#include <wstdisplay/texture.hpp>

namespace wstdisplay {

namespace {

struct GlyphPage
{
  Unique<Texture> texture = {};
  RectPacker packer;
};

/** Draw the 8 bit coverage \a brush onto \a target at \a x_pos,
    \a y_pos as white, with the border of \a effect around it */
void blit_glyph(FontEffect const& effect, surf::SoftwareSurface& target, FT_Bitmap const& brush,
                int x_pos, int y_pos)
{
  int const border = effect.border;
  x_pos += border;
  y_pos += border;

  int const start_x = std::max(0, -x_pos);
  int const start_y = std::max(0, -y_pos);
  int const end_x = std::min(static_cast<int>(brush.width), target.get_width() - x_pos);
  int const end_y = std::min(static_cast<int>(brush.rows), target.get_height() - y_pos);

  uint8_t* const target_buf = static_cast<uint8_t*>(target.get_data());
  int const target_pitch = target.get_pitch();

  if (border == 0) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = start_x; x < end_x; ++x) {
        uint8_t* const px = target_buf + (y + y_pos) * target_pitch + 4 * (x + x_pos);
        px[0] = px[1] = px[2] = 255;
        px[3] = brush.buffer[y * brush.pitch + x];
      }
    }
    return;
  }

  // the border, a diamond around every pixel of the glyph
  uint8_t const border_value = effect.outline ? 0 : 255;
  for (int y = start_y; y < end_y; ++y) {
    for (int x = start_x; x < end_x; ++x) {
      int const coverage = brush.buffer[y * brush.pitch + x];
      for (int by = -border; by <= border; ++by) {
        for (int bx = -border + std::abs(by); bx <= border - std::abs(by); ++bx) {
          uint8_t* const px = target_buf + (y + y_pos + by) * target_pitch + 4 * (x + x_pos + bx);
          px[0] = px[1] = px[2] = border_value;
          px[3] = static_cast<uint8_t>(std::min(px[3] + coverage, 255));
        }
      }
    }
  }

  if (effect.outline) {
    // the glyph itself on top of the border
    for (int y = start_y; y < end_y; ++y) {
      for (int x = start_x; x < end_x; ++x) {
        int const alpha = brush.buffer[y * brush.pitch + x];
        uint8_t* const px = target_buf + (y + y_pos) * target_pitch + 4 * (x + x_pos);
        for (int c = 0; c < 3; ++c) {
          px[c] = static_cast<uint8_t>((px[c] * (255 - alpha) + alpha * 255) / 255);
        }
        px[3] = static_cast<uint8_t>(std::min(px[3] + alpha, 255));
      }
    }
  }
}

} // namespace

class TTFFontImpl
{
public:
  Device& device;
  std::vector<char> buffer;
  FT_Face face = nullptr;
  int size = 0;
  FontEffect effect;
  float line_height = 0.0f;

  std::vector<GlyphPage> pages;
  geom::isize page_size;
  mutable std::unordered_map<char32_t, Glyph> glyphs;
  mutable std::unordered_map<char32_t, bool> missing;

  explicit TTFFontImpl(Device& device_) :
    device(device_),
    buffer(),
    effect(),
    pages(),
    page_size(512, 512),
    glyphs(),
    missing()
  {}

  TTFFontImpl(TTFFontImpl const&) = delete;
  TTFFontImpl& operator=(TTFFontImpl const&) = delete;

  ~TTFFontImpl()
  {
    if (face) {
      FT_Done_Face(face);
    }
  }

  Glyph const* load_glyph(char32_t codepoint)
  {
    FT_UInt const index = FT_Get_Char_Index(face, codepoint);
    if (index == 0 && codepoint != 0) {
      return nullptr;
    }

    if (FT_Load_Glyph(face, index, FT_LOAD_RENDER)) {
      log_warn("TTFFont: couldn't render U+{:04X}", static_cast<uint32_t>(codepoint));
      return nullptr;
    }

    FT_GlyphSlot slot = face->glyph;
    float const advance = static_cast<float>(slot->advance.x >> 6);

    int const bitmap_w = static_cast<int>(slot->bitmap.width);
    int const bitmap_h = static_cast<int>(slot->bitmap.rows);
    if (bitmap_w == 0 || bitmap_h == 0) {
      // whitespace
      return &(glyphs[codepoint] = Glyph{{}, {}, {}, advance});
    }


    int const glyph_w = bitmap_w + 2 * effect.border;
    int const glyph_h = bitmap_h + 2 * effect.border;

    // one pixel of padding around each glyph, filled by generate_border()
    surf::SoftwareSurface image = surf::SoftwareSurface::create(
      surf::PixelFormat::RGBA8, geom::isize(glyph_w + 2, glyph_h + 2), surf::palette::transparent);
    blit_glyph(effect, image, slot->bitmap, 1, 1);
    generate_border(image, 1, 1, glyph_w, glyph_h);

    auto [page, rect] = allocate(image.get_size());
    page->texture->put(image, rect.topleft());

    float const tw = static_cast<float>(page->texture->get_width());
    float const th = static_cast<float>(page->texture->get_height());
    geom::frect const uv(static_cast<float>(rect.left() + 1) / tw,
                         static_cast<float>(rect.top() + 1) / th,
                         static_cast<float>(rect.left() + 1 + glyph_w) / tw,
                         static_cast<float>(rect.top() + 1 + glyph_h) / th);

    float const x = static_cast<float>(slot->bitmap_left - effect.border);
    float const y = static_cast<float>(-slot->bitmap_top - effect.border);
    geom::frect const glyph_rect(x, y, x + static_cast<float>(glyph_w), y + static_cast<float>(glyph_h));

    return &(glyphs[codepoint] = Glyph{page->texture, uv, glyph_rect, advance});
  }

  std::pair<GlyphPage*, geom::irect> allocate(geom::isize const& glyph_size)
  {
    for (auto& page : pages) {
      if (auto rect = page.packer.allocate(glyph_size)) {
        return {&page, *rect};
      }
    }

    pages.push_back(GlyphPage{device.create_texture(page_size), RectPacker(page_size)});
    auto rect = pages.back().packer.allocate(glyph_size);
    if (!rect) {
      throw std::runtime_error("TTFFont: glyph too large for atlas page");
    }
    return {&pages.back(), *rect};
  }
};

TTFFont::TTFFont(Device& device, FT_Library freetype, std::filesystem::path const& filename, int size,
                 FontEffect const& effect) :
  impl(std::make_unique<TTFFontImpl>(device))
{
  if (size <= 0) {
    throw std::invalid_argument("TTFFont: size must be positive");
  }
  if (effect.border < 0) {
    throw std::invalid_argument("TTFFont: border must not be negative");
  }

  impl->size = size;
  impl->effect = effect;

  std::ifstream fin(filename, std::ios::binary);
  if (!fin) {
    throw std::runtime_error("TTFFont: couldn't open " + filename.string());
  }
  impl->buffer.assign(std::istreambuf_iterator<char>(fin), std::istreambuf_iterator<char>());

  if (FT_New_Memory_Face(freetype,
                         reinterpret_cast<FT_Byte const*>(impl->buffer.data()),
                         static_cast<FT_Long>(impl->buffer.size()),
                         0, &impl->face))
  {
    throw std::runtime_error("TTFFont: couldn't load font: " + filename.string());
  }

  FT_Set_Pixel_Sizes(impl->face, static_cast<FT_UInt>(size), static_cast<FT_UInt>(size));
  FT_Select_Charmap(impl->face, FT_ENCODING_UNICODE);

  impl->line_height = static_cast<float>(impl->face->size->metrics.height >> 6) +
    static_cast<float>(2 * effect.border);

  // a page that fits a reasonable set of glyphs, larger fonts get larger pages
  int const page = std::clamp(size * 16, 256, 2048);
  impl->page_size = geom::isize(page, page);

  for (char32_t cp = 32; cp < 256; ++cp) {
    get_glyph(cp);
  }
}

TTFFont::~TTFFont()
{
}

Glyph const*
TTFFont::get_glyph(char32_t codepoint) const
{
  auto it = impl->glyphs.find(codepoint);
  if (it != impl->glyphs.end()) {
    return &it->second;
  }

  if (impl->missing.contains(codepoint)) {
    return nullptr;
  }

  Glyph const* glyph = impl->load_glyph(codepoint);
  if (!glyph) {
    impl->missing[codepoint] = true;
  }
  return glyph;
}

float
TTFFont::get_height() const
{
  return static_cast<float>(impl->size + 2 * impl->effect.border);
}

float
TTFFont::get_line_height() const
{
  return impl->line_height;
}

} // namespace wstdisplay

/* EOF */
