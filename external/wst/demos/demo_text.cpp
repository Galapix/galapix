// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Fonts: TTFFont at different sizes and with a border effect,
// transformed text, a BitmapFont from a generated pixel font image
// and a TextArea with markup and letter by letter display.

#include <array>
#include <cmath>
#include <format>
#include <string_view>

#include <surf/palette.hpp>
#include <wstdisplay/font/bitmap_font.hpp>
#include <wstdisplay/font/text_area.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

namespace {

// 5x7 pixel font, one byte per row, the low 5 bits are the pixels
constexpr std::string_view pixel_charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,:!?-";
constexpr std::array<std::array<uint8_t, 7>, 43> pixel_glyphs = {{
  {0x0e, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}, // A
  {0x1e, 0x11, 0x11, 0x1e, 0x11, 0x11, 0x1e}, // B
  {0x0e, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0e}, // C
  {0x1e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1e}, // D
  {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x1f}, // E
  {0x1f, 0x10, 0x10, 0x1e, 0x10, 0x10, 0x10}, // F
  {0x0e, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0f}, // G
  {0x11, 0x11, 0x11, 0x1f, 0x11, 0x11, 0x11}, // H
  {0x0e, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0e}, // I
  {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0c}, // J
  {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, // K
  {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}, // L
  {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11}, // M
  {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11}, // N
  {0x0e, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, // O
  {0x1e, 0x11, 0x11, 0x1e, 0x10, 0x10, 0x10}, // P
  {0x0e, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0d}, // Q
  {0x1e, 0x11, 0x11, 0x1e, 0x14, 0x12, 0x11}, // R
  {0x0f, 0x10, 0x10, 0x0e, 0x01, 0x01, 0x1e}, // S
  {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}, // T
  {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0e}, // U
  {0x11, 0x11, 0x11, 0x11, 0x11, 0x0a, 0x04}, // V
  {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0a}, // W
  {0x11, 0x11, 0x0a, 0x04, 0x0a, 0x11, 0x11}, // X
  {0x11, 0x11, 0x11, 0x0a, 0x04, 0x04, 0x04}, // Y
  {0x1f, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1f}, // Z
  {0x0e, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0e}, // 0
  {0x04, 0x0c, 0x04, 0x04, 0x04, 0x04, 0x0e}, // 1
  {0x0e, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1f}, // 2
  {0x1f, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0e}, // 3
  {0x02, 0x06, 0x0a, 0x12, 0x1f, 0x02, 0x02}, // 4
  {0x1f, 0x10, 0x1e, 0x01, 0x01, 0x11, 0x0e}, // 5
  {0x06, 0x08, 0x10, 0x1e, 0x11, 0x11, 0x0e}, // 6
  {0x1f, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}, // 7
  {0x0e, 0x11, 0x11, 0x0e, 0x11, 0x11, 0x0e}, // 8
  {0x0e, 0x11, 0x11, 0x0f, 0x01, 0x02, 0x0c}, // 9
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
  {0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x0c}, // .
  {0x00, 0x00, 0x00, 0x00, 0x0c, 0x04, 0x08}, // ,
  {0x00, 0x0c, 0x0c, 0x00, 0x0c, 0x0c, 0x00}, // :
  {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04}, // !
  {0x0e, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04}, // ?
  {0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00}, // -
}};

/** The pixel font as an image with 8x8 cells, 16 per row, with a
    drop shadow baked in */
surf::SoftwareSurface make_pixel_font()
{
  geom::isize const cell(8, 8);
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8,
                                                              geom::isize(16 * cell.width(), 3 * cell.height()),
                                                              surf::palette::transparent);
  for (size_t i = 0; i < pixel_glyphs.size(); ++i) {
    int const cx = static_cast<int>(i % 16) * cell.width();
    int const cy = static_cast<int>(i / 16) * cell.height();
    for (int pass = 0; pass < 2; ++pass) {
      for (int y = 0; y < 7; ++y) {
        for (int x = 0; x < 5; ++x) {
          if (pixel_glyphs[i][static_cast<size_t>(y)] & (0x10 >> x)) {
            if (pass == 0) {
              image.put_pixel(geom::ipoint(cx + x + 1, cy + y + 1), surf::Color(0.0f, 0.0f, 0.0f, 0.6f));
            } else {
              image.put_pixel(geom::ipoint(cx + x, cy + y), surf::palette::white);
            }
          }
        }
      }
    }
  }
  return image;
}

} // namespace

class TextDemo : public wstdemo::DemoApp
{
public:
  TextDemo(int argc, char** argv) :
    DemoApp("Text", argc, argv),
    m_canvas(),
    m_fonts(),
    m_title(),
    m_pixel_font(),
    m_text_area(),
    m_restart(0.0f)
  {}

  void init() override
  {
    FontManager& fonts = get_font_manager();
    for (int size : {10, 12, 16, 20, 28, 40}) {
      m_fonts.push_back(fonts.load(get_data("Vera.ttf"), size));
    }
    m_title = fonts.load(get_data("Vera.ttf"), 48, FontEffect{.border = 3, .outline = true});
    m_pixel_font = fonts.add(BitmapFont::from_grid(get_device(), make_pixel_font(), geom::isize(8, 8),
                                                   pixel_charset, true, 1.0f));

    m_text_area = std::make_unique<TextArea>(&fonts.get(m_fonts[3]), geom::frect(700, 330, 1240, 520), true);
    init_text();
  }

  void update(float delta) override
  {
    m_text_area->update(delta);
    if (m_text_area->is_progress_complete()) {
      m_restart += delta;
      if (m_restart > 2.0f) {
        m_restart = 0.0f;
        m_text_area = std::make_unique<TextArea>(&get_font_manager().get(m_fonts[3]),
                                                 geom::frect(700, 330, 1240, 520), true);
        init_text();
      }
    }
  }

  void init_text()
  {
    m_text_area->set_text(
      "A <b>TextArea</b> wraps words to its rectangle and understands a bit of markup: "
      "<i>italic blue</i>, <small>small text</small>, <large>large</large> and "
      "<sin>wavy text</sin>.<sleep/> It can also show the text letter by letter, "
      "like a dialog in a game.");
  }

  void draw(Renderer& renderer) override
  {
    float const t = get_time();
    geom::fsize const size(get_size());

    FontManager const& fonts = get_font_manager();
    Font const& pixel_font = fonts.get(m_pixel_font);

    m_canvas.clear();
    float const s = std::min(size.width() / 1280.0f, size.height() / 720.0f);
    m_canvas.scale(s, s);

    m_canvas.draw_text(fonts.get(m_title), geom::fpoint(640, 70), "Fonts in wstdisplay",
                       surf::palette::gold, TextAlign::Center);

    float y = 140.0f;
    for (FontId const id : m_fonts) {
      Font const& font = fonts.get(id);
      m_canvas.draw_text(font, geom::fpoint(40, y),
                         std::format("{}px: The quick brown fox jumps over the lazy dog", font.get_height()));
      y += font.get_line_height() + 4.0f;
    }

    // transformed text
    m_canvas.save();
    m_canvas.translate(1000, 200);
    m_canvas.rotate(std::sin(t) * 15.0f);
    m_canvas.scale(1.0f + 0.1f * std::sin(t * 2.0f), 1.0f + 0.1f * std::sin(t * 2.0f));
    m_canvas.draw_text(fonts.get(m_fonts[4]), geom::fpoint(0, 0), "rotated\nand scaled", surf::palette::lightgreen,
                       TextAlign::Center);
    m_canvas.restore();

    // pixel font, scaled up with nearest filtering
    m_canvas.fill_rect(geom::frect(30, 470, 650, 680), surf::Color(0.2f, 0.25f, 0.4f));
    m_canvas.draw_text(pixel_font, geom::fpoint(50, 500), "BITMAP FONT FROM A GRID IMAGE", surf::palette::white);
    m_canvas.save();
    m_canvas.translate(50, 560);
    m_canvas.scale(3.0f, 3.0f);
    m_canvas.draw_text(pixel_font, geom::fpoint(0, 0), "SCORE: 0012345", surf::palette::yellow);
    m_canvas.draw_text(pixel_font, geom::fpoint(0, 12), "LIVES: 3   TIME: 99", surf::palette::orangered);
    m_canvas.restore();

    // text area
    m_canvas.fill_rounded_rect(geom::frect(680, 300, 1260, 540), 10.0f, surf::Color(0.0f, 0.0f, 0.0f, 0.5f));
    m_text_area->draw(m_canvas);

    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.12f, 0.12f, 0.16f)});
  }

private:
  Canvas m_canvas;
  std::vector<FontId> m_fonts;
  FontId m_title;
  FontId m_pixel_font;
  std::unique_ptr<TextArea> m_text_area;
  float m_restart;
};

WST_DEMO_MAIN(TextDemo)

/* EOF */
