// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Sprite benchmark: many bouncing sprites of a few kinds.
//
//   --count N   number of sprites (default 20000)
//   --no-atlas, --group, --unordered, --fancy
//               start with the toggles below set
//   + / -       add / remove 5000 sprites
//   a           toggle atlas vs. one texture per sprite kind
//   z           toggle grouping the kinds by z
//   u           toggle an unordered layer, which groups by texture
//   r           toggle rotation and tinting

#include <cmath>
#include <format>
#include <random>
#include <string>
#include <vector>

#include <surf/palette.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

namespace {

/** A round sprite with a colored rim, generated so the demo doesn't
    need image files */
surf::SoftwareSurface make_ball(int size, surf::Color const& color)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(size, size));
  float const r = static_cast<float>(size) / 2.0f;
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      float const dx = static_cast<float>(x) + 0.5f - r;
      float const dy = static_cast<float>(y) + 0.5f - r;
      float const d = std::sqrt(dx * dx + dy * dy) / r;
      float const alpha = std::clamp((1.0f - d) * r, 0.0f, 1.0f);
      float const shade = 1.0f - 0.5f * d * d;
      image.put_pixel(geom::ipoint(x, y), surf::Color(color.r * shade, color.g * shade, color.b * shade, alpha));
    }
  }
  return image;
}

struct Sprite
{
  float x;
  float y;
  float vx;
  float vy;
  float angle;
  int kind;
};

} // namespace

class SpritesDemo : public wstdemo::DemoApp
{
public:
  SpritesDemo(int argc, char** argv) :
    DemoApp("Sprites", argc, argv),
    m_count(20000),
    m_canvas(),
    m_surfaces(),
    m_atlas_kinds(),
    m_textures(),
    m_texture_kinds(),
    m_sprites(),
    m_rng(42),
    m_use_atlas(true),
    m_group_by_z(false),
    m_unordered(false),
    m_fancy(false)
  {}

  bool parse_option(std::string_view arg, std::function<std::string_view()> const& next) override
  {
    if (arg == "--count") {
      m_count = std::stoi(std::string(next()));
    } else if (arg == "--no-atlas") {
      m_use_atlas = false;
    } else if (arg == "--group") {
      m_group_by_z = true;
    } else if (arg == "--unordered") {
      m_unordered = true;
    } else if (arg == "--fancy") {
      m_fancy = true;
    } else {
      return false;
    }
    return true;
  }

  std::string get_usage() const override
  {
    return "  --count N            number of sprites\n"
           "  --no-atlas           one texture per sprite kind\n"
           "  --group              group sprite kinds by z\n"
           "  --unordered          let the renderer group by texture\n"
           "  --fancy              rotate and tint the sprites\n";
  }

  void init() override
  {
    m_surfaces = std::make_unique<SurfaceManager>(get_device());

    surf::SoftwareSurface const tux = surf::SoftwareSurface::from_file(get_data("tux.png"));
    std::vector<surf::SoftwareSurface> images = {
      tux,
      make_ball(32, surf::palette::red),
      make_ball(32, surf::palette::lime),
      make_ball(32, surf::palette::dodgerblue),
      make_ball(24, surf::palette::gold),
      make_ball(16, surf::palette::white),
    };

    for (auto const& image : images) {
      m_atlas_kinds.push_back(m_surfaces->create(image));
      m_textures.push_back(get_device().create_texture(image));
      m_texture_kinds.push_back(Surface(m_textures.back(), image.get_size()));
    }

    resize(m_count);
  }

  void resize(int count)
  {
    geom::isize const size = get_size();
    std::uniform_real_distribution<float> px(0.0f, static_cast<float>(size.width()));
    std::uniform_real_distribution<float> py(0.0f, static_cast<float>(size.height()) / 2.0f);
    std::uniform_real_distribution<float> v(-200.0f, 200.0f);
    std::uniform_int_distribution<int> kind(0, static_cast<int>(m_atlas_kinds.size()) - 1);

    while (static_cast<int>(m_sprites.size()) < count) {
      m_sprites.push_back(Sprite{px(m_rng), py(m_rng), v(m_rng), v(m_rng), 0.0f, kind(m_rng)});
    }
    m_sprites.resize(static_cast<size_t>(std::max(0, count)));
    m_count = count;
  }

  void on_event(SDL_Event const& event) override
  {
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym)
      {
        case SDLK_PLUS:
        case SDLK_KP_PLUS:
        case SDLK_EQUALS:
          resize(m_count + 5000);
          break;

        case SDLK_MINUS:
        case SDLK_KP_MINUS:
          resize(std::max(0, m_count - 5000));
          break;

        case SDLK_a: m_use_atlas = !m_use_atlas; break;
        case SDLK_z: m_group_by_z = !m_group_by_z; break;
        case SDLK_u: m_unordered = !m_unordered; break;
        case SDLK_r: m_fancy = !m_fancy; break;
        default: break;
      }
    }
  }

  void update(float delta) override
  {
    geom::fsize const size(get_size());
    for (auto& s : m_sprites) {
      s.vy += 400.0f * delta;
      s.x += s.vx * delta;
      s.y += s.vy * delta;
      s.angle += s.vx * delta;

      if (s.x < 0.0f) { s.x = 0.0f; s.vx = -s.vx; }
      if (s.x > size.width()) { s.x = size.width(); s.vx = -s.vx; }
      if (s.y > size.height()) { s.y = size.height(); s.vy = -std::abs(s.vy) * 0.9f - 50.0f; }
      if (s.y < 0.0f) { s.y = 0.0f; s.vy = 0.0f; }
    }
  }

  std::string get_info() const override
  {
    return std::format("{} sprites, atlas: {}, grouped by z: {}, unordered: {}, rotation/tint: {}\n"
                       "keys: +/- count, a atlas, z grouping, u unordered, r rotation",
                       m_sprites.size(), m_use_atlas, m_group_by_z, m_unordered, m_fancy);
  }

  void draw(Renderer& renderer) override
  {
    auto const& kinds = m_use_atlas ? m_atlas_kinds : m_texture_kinds;

    m_canvas.clear();
    for (size_t kind = 0; kind < kinds.size(); ++kind) {
      m_canvas.set_layer_unordered(static_cast<float>(kind), m_unordered);
    }

    for (auto const& s : m_sprites)
    {
      Surface const& surface = kinds[static_cast<size_t>(s.kind)];
      m_canvas.set_z(m_group_by_z ? static_cast<float>(s.kind) : 0.0f);

      if (m_fancy) {
        m_canvas.draw(surface, DrawParams()
                      .set_pos(geom::fpoint(s.x, s.y))
                      .set_anchor(geom::origin::BOTTOM_CENTER)
                      .set_angle(s.angle)
                      .set_color(surf::Color(0.5f + 0.5f * std::sin(s.x * 0.01f), 1.0f, 0.5f + 0.5f * std::cos(s.y * 0.01f))));
      } else {
        m_canvas.draw(surface, geom::fpoint(s.x - surface.get_width() / 2.0f, s.y - surface.get_height()));
      }
    }

    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.1f, 0.1f, 0.15f)});
  }

private:
  int m_count;
  Canvas m_canvas;
  std::unique_ptr<SurfaceManager> m_surfaces;
  std::vector<Surface> m_atlas_kinds;
  std::vector<Unique<Texture>> m_textures;
  std::vector<Surface> m_texture_kinds;
  std::vector<Sprite> m_sprites;
  std::mt19937 m_rng;
  bool m_use_atlas;
  bool m_group_by_z;
  bool m_unordered;
  bool m_fancy;
};

WST_DEMO_MAIN(SpritesDemo)

/* EOF */
