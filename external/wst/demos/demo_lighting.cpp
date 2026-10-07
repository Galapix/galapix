// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Compositor with the lighting passes: a world drawn into "color",
// lights into the quarter resolution "light" pass that is multiplied
// with the screen, glows into "highlight" and a HUD into "overlay".
//
//   mouse wheel   zoom around the mouse pointer
//   q / e         rotate the camera
//   1 - 4         toggle the passes
//   space         toggle ambient light

#include <array>
#include <cmath>
#include <format>
#include <random>
#include <vector>

#include <surf/palette.hpp>
#include <wstdisplay/compositor.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>
#include <wstdisplay/view.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

namespace {

/** Radial falloff from white in the center to transparent */
surf::SoftwareSurface make_light(int size)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(size, size));
  float const r = static_cast<float>(size) / 2.0f;
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      float const dx = (static_cast<float>(x) + 0.5f - r) / r;
      float const dy = (static_cast<float>(y) + 0.5f - r) / r;
      float const d = std::clamp(1.0f - std::sqrt(dx * dx + dy * dy), 0.0f, 1.0f);
      image.put_pixel(geom::ipoint(x, y), surf::Color(1.0f, 1.0f, 1.0f, d * d));
    }
  }
  return image;
}

/** A brick tile so the world has some structure */
surf::SoftwareSurface make_brick(int size)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(size, size));
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      int const row = y / (size / 4);
      int const shift = (row % 2) * size / 4;
      bool const mortar = (y % (size / 4) < 2) || ((x + shift) % (size / 2) < 2);
      float const noise = static_cast<float>((x * 7 + y * 13) % 11) / 40.0f;
      surf::Color const c = mortar ? surf::Color(0.55f, 0.55f, 0.5f) :
        surf::Color(0.6f + noise, 0.3f + noise / 2.0f, 0.2f);
      image.put_pixel(geom::ipoint(x, y), c);
    }
  }
  return image;
}

struct Light
{
  geom::fpoint center;
  float radius;
  float speed;
  float size;
  surf::Color color;
};

} // namespace

class LightingDemo : public wstdemo::DemoApp
{
public:
  LightingDemo(int argc, char** argv) :
    DemoApp("Lighting", argc, argv),
    m_compositor(Compositor::lighting_passes(surf::Color(0.1f, 0.1f, 0.2f))),
    m_view(),
    m_surfaces(),
    m_light(),
    m_brick(),
    m_tux(),
    m_lights(),
    m_rotation(0.0f),
    m_ambient(true)
  {}

  void init() override
  {
    m_surfaces = std::make_unique<SurfaceManager>(get_device());
    m_light = m_surfaces->create(make_light(256));
    m_brick = m_surfaces->create(make_brick(64));
    m_tux = m_surfaces->get(get_data("tux.png"));

    std::mt19937 rng(7);
    std::uniform_real_distribution<float> pos(-800.0f, 800.0f);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int i = 0; i < 24; ++i) {
      m_lights.push_back(Light{
          geom::fpoint(pos(rng), pos(rng)),
          50.0f + 150.0f * unit(rng),
          0.3f + unit(rng),
          1.0f + 2.0f * unit(rng),
          surf::Color(0.4f + 0.6f * unit(rng), 0.4f + 0.6f * unit(rng), 0.4f + 0.6f * unit(rng))
        });
    }

    m_view = View(get_size());
    m_view.set_pos(geom::fpoint(0.0f, 0.0f));
    m_view.set_zoom(0.6f);
  }

  void on_resize(geom::isize const& size) override
  {
    m_view.set_size(size);
  }

  void on_event(SDL_Event const& event) override
  {
    if (event.type == SDL_MOUSEWHEEL) {
      int mx, my;
      SDL_GetMouseState(&mx, &my);
      float const factor = event.wheel.y > 0 ? 1.1f : 1.0f / 1.1f;
      m_view.set_zoom(geom::fpoint(static_cast<float>(mx), static_cast<float>(my)), m_view.get_zoom() * factor);
    } else if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym)
      {
        case SDLK_1: toggle("color"); break;
        case SDLK_2: toggle("light"); break;
        case SDLK_3: toggle("highlight"); break;
        case SDLK_4: toggle("overlay"); break;
        case SDLK_SPACE:
          m_ambient = !m_ambient;
          m_compositor.get_pass("light").clear_color = m_ambient ?
            surf::Color(0.1f, 0.1f, 0.2f) : surf::Color(0.0f, 0.0f, 0.0f);
          break;
        default:
          break;
      }
    }
  }

  void toggle(std::string_view pass)
  {
    m_compositor.get_pass(pass).enabled = !m_compositor.get_pass(pass).enabled;
  }

  void update(float delta) override
  {
    Uint8 const* keys = SDL_GetKeyboardState(nullptr);
    if (keys[SDL_SCANCODE_Q]) { m_rotation -= 45.0f * delta; }
    if (keys[SDL_SCANCODE_E]) { m_rotation += 45.0f * delta; }
    m_view.set_rotation(m_rotation);
  }

  std::string get_info() const override
  {
    return std::format("zoom {:.2f}, rotation {:.0f}\n"
                       "wheel zoom, q/e rotate, 1-4 toggle passes, space ambient light",
                       m_view.get_zoom(), m_rotation);
  }

  void draw(Renderer& renderer) override
  {
    float const t = get_time();

    renderer.clear(surf::palette::black);

    // the compositor clears the canvases including their state after rendering
    Canvas& color = m_compositor.get_canvas("color");
    Canvas& light = m_compositor.get_canvas("light");
    Canvas& highlight = m_compositor.get_canvas("highlight");
    Canvas& overlay = m_compositor.get_canvas("overlay");

    // only draw the tiles that are visible
    geom::frect const visible = m_view.get_clip_rect();
    float const tile = 64.0f;
    for (float y = std::floor(visible.top() / tile) * tile; y < visible.bottom(); y += tile) {
      for (float x = std::floor(visible.left() / tile) * tile; x < visible.right(); x += tile) {
        color.draw(m_brick, geom::fpoint(x, y));
      }
    }

    color.set_z(1.0f);
    for (int i = 0; i < 12; ++i) {
      float const a = static_cast<float>(i) * 0.5236f + t * 0.2f;
      color.draw(m_tux, DrawParams().set_pos(geom::fpoint(std::cos(a) * 500.0f, std::sin(a) * 500.0f))
                 .set_anchor(geom::origin::CENTER));
    }

    light.set_blend(Blend::Add);
    highlight.set_blend(Blend::Add);
    for (auto const& l : m_lights) {
      geom::fpoint const pos(l.center.x() + std::cos(t * l.speed) * l.radius,
                             l.center.y() + std::sin(t * l.speed * 1.3f) * l.radius);
      light.draw(m_light, DrawParams().set_pos(pos).set_anchor(geom::origin::CENTER)
                 .set_scale(l.size).set_color(l.color));
      highlight.draw(m_light, DrawParams().set_pos(pos).set_anchor(geom::origin::CENTER)
                     .set_scale(0.15f).set_color(l.color));
    }

    // the overlay pass ignores the camera
    overlay.fill_rounded_rect(geom::frect(20, static_cast<float>(get_size().height()) - 60,
                                          420, static_cast<float>(get_size().height()) - 20),
                              8.0f, surf::Color(0.0f, 0.0f, 0.0f, 0.6f));
    overlay.draw_text(get_font(), geom::fpoint(36, static_cast<float>(get_size().height()) - 35),
                      "overlay pass: not affected by camera or light", surf::palette::white);

    m_compositor.render(renderer, m_view);
  }

private:
  Compositor m_compositor;
  View m_view;
  std::unique_ptr<SurfaceManager> m_surfaces;
  Surface m_light;
  Surface m_brick;
  Surface m_tux;
  std::vector<Light> m_lights;
  float m_rotation;
  bool m_ambient;
};

WST_DEMO_MAIN(LightingDemo)

/* EOF */
