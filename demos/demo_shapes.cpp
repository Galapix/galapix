// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Every Canvas primitive: shapes, outlines, gradients, transforms,
// clipping, text and surface drawing parameters.

#include <array>
#include <cmath>

#include <geom/quad.hpp>
#include <surf/palette.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

class ShapesDemo : public wstdemo::DemoApp
{
public:
  ShapesDemo(int argc, char** argv) :
    DemoApp("Shapes", argc, argv),
    m_canvas(),
    m_surfaces(),
    m_tux()
  {}

  void init() override
  {
    m_surfaces = std::make_unique<SurfaceManager>(get_device());
    m_tux = m_surfaces->get(get_data("tux.png"));
  }

  void draw(Renderer& renderer) override
  {
    float const t = get_time();
    geom::fsize const size(get_size());

    m_canvas.clear();

    // the scene is laid out for 1280x720 and scaled to the window
    float const s = std::min(size.width() / 1280.0f, size.height() / 720.0f);
    m_canvas.scale(s, s);

    // row 1: filled shapes
    m_canvas.fill_rect(geom::frect(20, 20, 140, 120), surf::palette::firebrick);
    m_canvas.fill_rounded_rect(geom::frect(160, 20, 280, 120), 20.0f, surf::palette::seagreen);
    m_canvas.fill_circle(geom::fpoint(350, 70), 50.0f, surf::palette::royalblue);
    m_canvas.fill_arc(geom::fpoint(470, 70), 50.0f, -90.0f, -90.0f + 270.0f * (0.5f + 0.5f * std::sin(t)),
                      surf::palette::gold);
    m_canvas.fill_triangle(geom::fpoint(560, 120), geom::fpoint(620, 20), geom::fpoint(680, 120),
                           surf::palette::orchid);
    m_canvas.fill_quad(geom::fquad(geom::fpoint(700, 30), geom::fpoint(820, 20),
                                   geom::fpoint(800, 120), geom::fpoint(710, 110)),
                       surf::palette::darkorange);
    m_canvas.fill_vertical_gradient(geom::frect(840, 20, 960, 120), surf::palette::skyblue, surf::palette::navy);
    m_canvas.fill_gradient(geom::frect(980, 20, 1100, 120),
                           surf::palette::red, surf::palette::lime, surf::palette::blue, surf::palette::white);
    m_canvas.fill_rect(geom::frect(1120, 20, 1260, 120), surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

    // row 2: outlines with increasing width
    for (int i = 0; i < 4; ++i) {
      float const w = 1.0f + static_cast<float>(i) * 2.0f;
      float const x = 20.0f + static_cast<float>(i) * 160.0f;
      m_canvas.draw_rect(geom::frect(x, 150, x + 60, 210), surf::palette::white, w);
      m_canvas.draw_rounded_rect(geom::frect(x + 70, 150, x + 140, 210), 15.0f, surf::palette::lightgreen, w);
      m_canvas.draw_circle(geom::fpoint(x + 30, 260), 30.0f, surf::palette::lightblue, w);
      m_canvas.draw_arc(geom::fpoint(x + 105, 260), 30.0f, 0.0f, 270.0f, surf::palette::yellow, w);
    }

    std::array<geom::fpoint, 6> zigzag;
    for (size_t i = 0; i < zigzag.size(); ++i) {
      zigzag[i] = geom::fpoint(680.0f + static_cast<float>(i) * 50.0f,
                               (i % 2 == 0 ? 160.0f : 280.0f) + 10.0f * std::sin(t * 2.0f + static_cast<float>(i)));
    }
    m_canvas.draw_polyline(zigzag, surf::palette::hotpink, 6.0f);
    m_canvas.draw_quad(geom::fquad(geom::fpoint(1000, 160), geom::fpoint(1240, 170),
                                   geom::fpoint(1230, 290), geom::fpoint(1010, 280)),
                       surf::palette::cyan, 3.0f);
    for (int i = 0; i < 8; ++i) {
      float const a = t + static_cast<float>(i) * 0.785f;
      m_canvas.draw_line(geom::fpoint(1120, 225),
                         geom::fpoint(1120 + std::cos(a) * 50.0f, 225 + std::sin(a) * 50.0f),
                         surf::palette::white, 1.0f + static_cast<float>(i) * 0.5f);
    }

    // row 3: transforms, a rotating group of shapes
    m_canvas.save();
    m_canvas.translate(160, 440);
    m_canvas.rotate(t * 30.0f);
    m_canvas.scale(1.0f + 0.2f * std::sin(t), 1.0f);
    m_canvas.fill_rect(geom::frect(-80, -80, 80, 80), surf::palette::slateblue);
    m_canvas.draw_rect(geom::frect(-80, -80, 80, 80), surf::palette::white, 3.0f);
    m_canvas.fill_circle(geom::fpoint(0, 0), 30.0f, surf::palette::orange);
    m_canvas.restore();

    // clipping, the clip rect follows the transform
    geom::frect const clip(320, 340, 620, 540);
    m_canvas.draw_rect(clip, surf::palette::gray);
    m_canvas.save();
    m_canvas.clip(clip);
    for (int i = 0; i < 12; ++i) {
      float const x = 300.0f + std::fmod(t * 80.0f + static_cast<float>(i) * 40.0f, 360.0f);
      m_canvas.fill_circle(geom::fpoint(x, 440.0f + 80.0f * std::sin(static_cast<float>(i))), 25.0f,
                           surf::Color(0.2f + 0.06f * static_cast<float>(i), 0.8f, 0.5f));
    }
    m_canvas.restore();

    // surfaces with DrawParams, the first one flashes through the effect color
    float const sx = 700.0f;
    m_canvas.draw(m_tux, DrawParams().set_pos(geom::fpoint(sx, 340))
                  .set_effect(surf::Color(1.0f, 1.0f, 1.0f, std::max(0.0f, std::sin(t * 6.0f)))));
    m_canvas.draw(m_tux, DrawParams().set_pos(geom::fpoint(sx + 200, 440))
                  .set_anchor(geom::origin::CENTER).set_angle(t * 90.0f));
    m_canvas.draw(m_tux, DrawParams().set_pos(geom::fpoint(sx + 320, 340)).set_hflip(true)
                  .set_color(surf::Color(1.0f, 0.6f, 0.6f)));
    m_canvas.draw(m_tux, DrawParams().set_pos(geom::fpoint(sx + 470, 540))
                  .set_anchor(geom::origin::BOTTOM_CENTER).set_scale(glm::vec2(1.0f, 0.75f + 0.25f * std::sin(t * 4.0f)))
                  .set_alpha(0.5f + 0.5f * std::sin(t)));

    // text with alignment
    m_canvas.draw_line(geom::fpoint(640, 580), geom::fpoint(640, 700), surf::palette::gray);
    m_canvas.draw_text(get_font(), geom::fpoint(640, 600), "Left aligned", surf::palette::white, TextAlign::Left);
    m_canvas.draw_text(get_font(), geom::fpoint(640, 630), "Center aligned", surf::palette::yellow, TextAlign::Center);
    m_canvas.draw_text(get_font(), geom::fpoint(640, 660), "Right aligned", surf::palette::cyan, TextAlign::Right);
    m_canvas.draw_text(get_font(), geom::fpoint(20, 620), "UTF-8: Grüße, Ünïcödé, ½ ¿ © «»\nmissing glyphs: ☃ ★\nthird line",
                       surf::palette::white);

    // z ordering: drawn in reverse order, sorted by z
    m_canvas.save();
    for (int i = 0; i < 4; ++i) {
      float const x = 1000.0f + static_cast<float>(i) * 30.0f;
      m_canvas.set_z(static_cast<float>(4 - i));
      m_canvas.fill_rect(geom::frect(x, 580 + static_cast<float>(i) * 20, x + 80, 660 + static_cast<float>(i) * 20),
                         surf::Color(0.25f * static_cast<float>(i + 1), 0.3f, 1.0f - 0.2f * static_cast<float>(i)));
    }
    m_canvas.restore();

    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.15f, 0.15f, 0.2f)});
  }

private:
  Canvas m_canvas;
  std::unique_ptr<SurfaceManager> m_surfaces;
  Surface m_tux;
};

WST_DEMO_MAIN(ShapesDemo)

/* EOF */
