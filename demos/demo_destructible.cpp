// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Destructible terrain as in Pingus: the terrain image is kept on the
// CPU and split into chunk textures, carving or adding material
// modifies the CPU copy and re-uploads only the changed rectangle of
// the affected chunks with Texture::put().
//
//   left mouse    carve a hole
//   right mouse   add material
//   space         toggle automatic explosions

#include <algorithm>
#include <cmath>
#include <format>
#include <random>
#include <vector>

#include <surf/palette.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/texture.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

namespace {

constexpr int chunk_size = 256;

class Terrain
{
public:
  Terrain(Device& device, geom::isize const& size) :
    m_size(size),
    m_image(surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, size, surf::palette::transparent)),
    m_chunks(),
    m_columns((size.width() + chunk_size - 1) / chunk_size),
    m_uploaded(0)
  {
    generate();

    for (int y = 0; y < size.height(); y += chunk_size) {
      for (int x = 0; x < size.width(); x += chunk_size) {
        geom::irect const rect(x, y, std::min(x + chunk_size, size.width()), std::min(y + chunk_size, size.height()));
        Unique<Texture> texture = device.create_texture(rect.size(), TextureParams{TextureFilter::Nearest});
        texture->put(m_image, rect, geom::ipoint(0, 0));
        Surface const surface(texture, rect.size());
        m_chunks.push_back(Chunk{rect, std::move(texture), surface});
      }
    }
  }

  void generate()
  {
    for (int x = 0; x < m_size.width(); ++x) {
      float const fx = static_cast<float>(x);
      int const top = static_cast<int>(static_cast<float>(m_size.height()) * 0.45f +
                                       60.0f * std::sin(fx * 0.007f) + 25.0f * std::sin(fx * 0.031f));
      for (int y = std::max(0, top); y < m_size.height(); ++y) {
        int const depth = y - top;
        float const n = static_cast<float>((x * 7919 + y * 104729) % 97) / 400.0f;
        surf::Color c = depth < 6 ? surf::Color(0.3f + n, 0.75f + n, 0.25f) :
                        depth < 60 ? surf::Color(0.55f + n, 0.38f + n, 0.22f + n) :
                                     surf::Color(0.42f + n, 0.42f + n, 0.46f + n);
        m_image.put_pixel(geom::ipoint(x, y), c);
      }
    }
  }

  /** Clear (or fill) a circle and upload the changed region */
  void paint_circle(geom::fpoint const& center, float radius, bool add)
  {
    geom::irect const dirty = geom::irect(static_cast<int>(center.x() - radius) - 1,
                                          static_cast<int>(center.y() - radius) - 1,
                                          static_cast<int>(center.x() + radius) + 2,
                                          static_cast<int>(center.y() + radius) + 2);
    geom::irect const area = intersect(dirty, geom::irect(m_size));
    if (area.width() <= 0 || area.height() <= 0) {
      return;
    }

    for (int y = area.top(); y < area.bottom(); ++y) {
      auto* row = static_cast<uint8_t*>(m_image.get_row_data(y));
      for (int x = area.left(); x < area.right(); ++x) {
        float const dx = static_cast<float>(x) + 0.5f - center.x();
        float const dy = static_cast<float>(y) + 0.5f - center.y();
        float const d = std::sqrt(dx * dx + dy * dy);
        uint8_t* px = row + x * 4;
        if (add && d < radius) {
          float const n = static_cast<float>((x * 31 + y * 17) % 13) / 60.0f;
          px[0] = static_cast<uint8_t>(255.0f * (0.7f + n));
          px[1] = static_cast<uint8_t>(255.0f * (0.6f + n));
          px[2] = static_cast<uint8_t>(255.0f * (0.3f + n));
          px[3] = 255;
        } else if (!add) {
          if (d < radius) {
            px[3] = 0;
          } else if (d < radius + 3.0f && px[3] != 0) {
            // scorched rim
            px[0] = static_cast<uint8_t>(px[0] / 2);
            px[1] = static_cast<uint8_t>(px[1] / 2);
            px[2] = static_cast<uint8_t>(px[2] / 2);
          }
        }
      }
    }

    // upload the changed part of each touched chunk
    for (auto& chunk : m_chunks) {
      geom::irect const part = intersect(area, chunk.rect);
      if (part.width() > 0 && part.height() > 0) {
        chunk.texture->put(m_image, part,
                                          geom::ipoint(part.left() - chunk.rect.left(), part.top() - chunk.rect.top()));
        m_uploaded += part.width() * part.height();
      }
    }
  }

  void draw(Canvas& canvas) const
  {
    for (auto const& chunk : m_chunks) {
      canvas.draw(chunk.surface, geom::fpoint(chunk.rect.topleft()));
    }
  }

  int take_uploaded()
  {
    int const v = m_uploaded;
    m_uploaded = 0;
    return v;
  }

  geom::isize get_size() const { return m_size; }
  int chunk_count() const { return static_cast<int>(m_chunks.size()); }

private:
  static geom::irect intersect(geom::irect const& a, geom::irect const& b)
  {
    int const left = std::max(a.left(), b.left());
    int const top = std::max(a.top(), b.top());
    return geom::irect(left, top,
                       std::max(left, std::min(a.right(), b.right())),
                       std::max(top, std::min(a.bottom(), b.bottom())));
  }

private:
  struct Chunk
  {
    geom::irect rect;
    Unique<Texture> texture;
    Surface surface;
  };

  geom::isize m_size;
  surf::SoftwareSurface m_image;
  std::vector<Chunk> m_chunks;
  int m_columns;
  int m_uploaded;
};

struct Explosion
{
  geom::fpoint pos;
  float age;
};

} // namespace

class DestructibleDemo : public wstdemo::DemoApp
{
public:
  DestructibleDemo(int argc, char** argv) :
    DemoApp("Destructible", argc, argv),
    m_canvas(),
    m_terrain(),
    m_explosions(),
    m_rng(11),
    m_auto(true),
    m_timer(0.0f),
    m_uploaded(0)
  {}

  void init() override
  {
    m_terrain = std::make_unique<Terrain>(get_device(), geom::isize(2048, 1024));
  }

  geom::fpoint screen_to_terrain(int x, int y) const
  {
    geom::fsize const size(get_size());
    float const scale = std::min(size.width() / 2048.0f, size.height() / 1024.0f);
    return geom::fpoint(static_cast<float>(x) / scale, static_cast<float>(y) / scale);
  }

  void on_event(SDL_Event const& event) override
  {
    if (event.type == SDL_MOUSEBUTTONDOWN) {
      geom::fpoint const pos = screen_to_terrain(event.button.x, event.button.y);
      if (event.button.button == SDL_BUTTON_LEFT) {
        explode(pos);
      } else if (event.button.button == SDL_BUTTON_RIGHT) {
        m_terrain->paint_circle(pos, 40.0f, true);
      }
    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
      m_auto = !m_auto;
    }
  }

  void explode(geom::fpoint const& pos)
  {
    std::uniform_real_distribution<float> radius(20.0f, 60.0f);
    m_terrain->paint_circle(pos, radius(m_rng), false);
    m_explosions.push_back(Explosion{pos, 0.0f});
  }

  void update(float delta) override
  {
    m_timer += delta;
    if (m_auto && m_timer > 0.1f) {
      m_timer = 0.0f;
      std::uniform_real_distribution<float> x(0.0f, 2048.0f);
      std::uniform_real_distribution<float> y(350.0f, 900.0f);
      explode(geom::fpoint(x(m_rng), y(m_rng)));
    }

    for (auto& e : m_explosions) {
      e.age += delta;
    }
    std::erase_if(m_explosions, [](Explosion const& e) { return e.age > 0.5f; });

    m_uploaded = m_terrain->take_uploaded();
  }

  std::string get_info() const override
  {
    return std::format("{} chunk textures, {} pixels uploaded last frame\n"
                       "left click carve, right click add, space automatic explosions",
                       m_terrain->chunk_count(), m_uploaded);
  }

  void draw(Renderer& renderer) override
  {
    geom::fsize const size(get_size());
    float const scale = std::min(size.width() / 2048.0f, size.height() / 1024.0f);

    m_canvas.clear();
    m_canvas.scale(scale, scale);
    m_canvas.set_z(-1.0f);
    m_canvas.fill_vertical_gradient(geom::frect(0, 0, 2048, 1024),
                                    surf::Color(0.35f, 0.55f, 0.9f), surf::Color(0.85f, 0.9f, 1.0f));
    m_canvas.set_z(0.0f);
    m_terrain->draw(m_canvas);

    m_canvas.set_z(1.0f);
    for (auto const& e : m_explosions) {
      float const t = e.age / 0.5f;
      m_canvas.fill_circle(e.pos, 20.0f + 60.0f * t, surf::Color(1.0f, 0.8f - 0.6f * t, 0.2f, 1.0f - t));
    }

    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.5f, 0.7f, 0.9f)});
  }

private:
  Canvas m_canvas;
  std::unique_ptr<Terrain> m_terrain;
  std::vector<Explosion> m_explosions;
  std::mt19937 m_rng;
  bool m_auto;
  float m_timer;
  int m_uploaded;
};

WST_DEMO_MAIN(DestructibleDemo)

/* EOF */
