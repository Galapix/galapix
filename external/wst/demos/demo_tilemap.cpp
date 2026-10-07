// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// A large tilemap, either recorded into the canvas every frame
// (culled to the visible area) or prebuilt into StaticBatch chunks
// that stay on the GPU. Parallax layers use fill_pattern().
//
//   arrow keys    scroll
//   s             toggle static batches
//   c             toggle culling (only for immediate mode)

#include <cmath>
#include <format>
#include <random>
#include <vector>

#include <surf/palette.hpp>
#include <wstdisplay/static_batch.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>
#include <wstdisplay/view.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;

namespace {

constexpr int tile_size = 32;
constexpr int map_width = 1024;
constexpr int map_height = 128;
constexpr int chunk_size = 32;

surf::SoftwareSurface make_tile(int kind)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8,
                                                              geom::isize(tile_size, tile_size));
  surf::Color const base = (kind == 0) ? surf::Color(0.35f, 0.65f, 0.25f) :  // grass
                           (kind == 1) ? surf::Color(0.5f, 0.35f, 0.2f) :    // dirt
                           (kind == 2) ? surf::Color(0.45f, 0.45f, 0.5f) :   // stone
                                         surf::Color(0.9f, 0.75f, 0.2f);     // gold
  for (int y = 0; y < tile_size; ++y) {
    for (int x = 0; x < tile_size; ++x) {
      float const n = static_cast<float>((x * 37 + y * 91 + kind * 17) % 23) / 60.0f;
      float const edge = (x == 0 || y == 0) ? 1.15f : (x == tile_size - 1 || y == tile_size - 1) ? 0.8f : 1.0f;
      surf::Color c(std::min(1.0f, (base.r + n) * edge), std::min(1.0f, (base.g + n) * edge),
                    std::min(1.0f, (base.b + n) * edge));
      if (kind == 0 && y < 6) {
        c = surf::Color(0.3f, 0.85f - static_cast<float>(y) * 0.05f, 0.2f);
      }
      image.put_pixel(geom::ipoint(x, y), c);
    }
  }
  return image;
}

/** A repeating background layer, power-of-two for GLES 2.0 */
surf::SoftwareSurface make_hills(int size, surf::Color const& color, float freq, float height)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(size, size),
                                                              surf::palette::transparent);
  for (int x = 0; x < size; ++x) {
    float const fx = static_cast<float>(x) / static_cast<float>(size) * 6.2832f;
    float const h = height * (0.6f + 0.25f * std::sin(fx * freq) + 0.15f * std::sin(fx * freq * 3.0f));
    for (int y = size - static_cast<int>(h * static_cast<float>(size)); y < size; ++y) {
      image.put_pixel(geom::ipoint(x, y), color);
    }
  }
  return image;
}

} // namespace

class TilemapDemo : public wstdemo::DemoApp
{
public:
  TilemapDemo(int argc, char** argv) :
    DemoApp("Tilemap", argc, argv),
    m_canvas(),
    m_view(),
    m_surfaces(),
    m_tiles(),
    m_far_hills_texture(),
    m_far_hills(),
    m_near_hills_texture(),
    m_near_hills(),
    m_map(),
    m_chunks(),
    m_use_static(true),
    m_cull(true),
    m_scroll(0.0f, 0.0f)
  {}

  bool parse_option(std::string_view arg, std::function<std::string_view()> const&) override
  {
    if (arg == "--immediate") {
      m_use_static = false;
      return true;
    }
    return false;
  }

  void init() override
  {
    m_surfaces = std::make_unique<SurfaceManager>(get_device(), SurfaceManager::Options{
        .params = TextureParams{TextureFilter::Nearest}});
    for (int kind = 0; kind < 4; ++kind) {
      m_tiles.push_back(m_surfaces->create(make_tile(kind)));
    }

    TextureParams const repeat{TextureFilter::Linear, TextureWrap::Repeat, TextureWrap::Clamp};
    m_far_hills_texture = get_device().create_texture(make_hills(256, surf::Color(0.3f, 0.35f, 0.55f), 2.0f, 0.7f), repeat);
    m_far_hills = Surface(m_far_hills_texture, geom::isize(256, 256));
    m_near_hills_texture = get_device().create_texture(make_hills(256, surf::Color(0.2f, 0.3f, 0.35f), 3.0f, 0.5f), repeat);
    m_near_hills = Surface(m_near_hills_texture, geom::isize(256, 256));

    generate_map();
    build_chunks();

    m_view = View(get_size());
    m_camera_y = static_cast<float>(ground_height(0) * tile_size);
  }

  int ground_height(int x) const
  {
    for (int y = 0; y < map_height; ++y) {
      int const kind = m_map[static_cast<size_t>(y * map_width + x)];
      if (kind == 0 || kind == 1) {
        return y;
      }
    }
    return map_height;
  }

  void generate_map()
  {
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> ore(0, 40);
    m_map.assign(static_cast<size_t>(map_width * map_height), -1);
    for (int x = 0; x < map_width; ++x) {
      float const fx = static_cast<float>(x);
      int const ground = 40 + static_cast<int>(10.0f * std::sin(fx * 0.05f) + 6.0f * std::sin(fx * 0.13f));
      for (int y = ground; y < map_height; ++y) {
        int kind = (y == ground) ? 0 : (y < ground + 4) ? 1 : 2;
        if (kind == 2 && ore(rng) == 0) {
          kind = 3;
        }
        m_map[static_cast<size_t>(y * map_width + x)] = kind;
      }
      // floating platforms
      if (x % 37 < 6) {
        m_map[static_cast<size_t>((ground - 8 - (x / 37) % 5) * map_width + x)] = 2;
      }
    }
  }

  void draw_tiles(Canvas& canvas, int x0, int y0, int x1, int y1)
  {
    for (int y = std::max(0, y0); y < std::min(map_height, y1); ++y) {
      for (int x = std::max(0, x0); x < std::min(map_width, x1); ++x) {
        int const kind = m_map[static_cast<size_t>(y * map_width + x)];
        if (kind >= 0) {
          canvas.draw(m_tiles[static_cast<size_t>(kind)],
                      geom::fpoint(static_cast<float>(x * tile_size), static_cast<float>(y * tile_size)));
        }
      }
    }
  }

  void build_chunks()
  {
    Canvas canvas;
    for (int cy = 0; cy < map_height; cy += chunk_size) {
      for (int cx = 0; cx < map_width; cx += chunk_size) {
        canvas.clear();
        draw_tiles(canvas, cx, cy, cx + chunk_size, cy + chunk_size);
        Unique<StaticBatch> chunk = get_device().create_static_batch();
        chunk->build(canvas);
        m_chunks.push_back(std::move(chunk));
      }
    }
  }

  void on_resize(geom::isize const& size) override
  {
    m_view.set_size(size);
  }

  void on_event(SDL_Event const& event) override
  {
    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym)
      {
        case SDLK_s: m_use_static = !m_use_static; break;
        case SDLK_c: m_cull = !m_cull; break;
        default: break;
      }
    }
  }

  void update(float delta) override
  {
    Uint8 const* keys = SDL_GetKeyboardState(nullptr);
    glm::vec2 dir(1.0f, 0.0f); // auto scroll, up/down move away from the terrain
    if (keys[SDL_SCANCODE_LEFT]) { dir.x = -3.0f; }
    if (keys[SDL_SCANCODE_RIGHT]) { dir.x = 3.0f; }
    if (keys[SDL_SCANCODE_UP]) { dir.y = -3.0f; }
    if (keys[SDL_SCANCODE_DOWN]) { dir.y = 3.0f; }
    m_scroll += dir * 300.0f * delta;

    float const max_x = static_cast<float>(map_width * tile_size);
    m_scroll.x = std::fmod(std::fmod(m_scroll.x, max_x) + max_x, max_x);
    m_scroll.y = std::clamp(m_scroll.y, -800.0f, 800.0f);
    m_scroll.y *= (dir.y == 0.0f) ? std::max(0.0f, 1.0f - delta * 2.0f) : 1.0f;

    // follow the terrain
    geom::fsize const size(get_size());
    float const center_x = m_scroll.x + size.width() / 2.0f;
    int const column = std::clamp(static_cast<int>(center_x) / tile_size, 0, map_width - 1);
    float const ground = static_cast<float>(ground_height(column) * tile_size);
    m_camera_y += (ground - m_camera_y) * std::min(1.0f, delta * 2.0f);
    m_view.set_pos(geom::fpoint(center_x, m_camera_y + m_scroll.y));
  }

  std::string get_info() const override
  {
    return std::format("{}x{} tiles, mode: {}{}\narrows scroll, s static batches, c culling",
                       map_width, map_height,
                       m_use_static ? "static batches" : "immediate",
                       (!m_use_static && !m_cull) ? " without culling" : "");
  }

  void draw(Renderer& renderer) override
  {
    geom::fsize const size(get_size());

    m_canvas.clear();

    // sky and parallax layers in screen space
    m_canvas.save();
    m_canvas.set_space(Space::Screen);
    m_canvas.set_z(-10.0f);
    m_canvas.fill_vertical_gradient(geom::frect(0, 0, size.width(), size.height()),
                                    surf::Color(0.4f, 0.6f, 0.95f), surf::Color(0.8f, 0.9f, 1.0f));
    // the hill layers end at the horizon and continue as solid color below
    float const horizon = size.height() * 0.6f;
    m_canvas.scale(2.0f, 2.0f);
    m_canvas.set_z(-9.0f);
    m_canvas.fill_pattern(m_far_hills, geom::frect(0, horizon / 2.0f - 256, size.width() / 2.0f, horizon / 2.0f),
                          geom::foffset(-m_scroll.x * 0.1f, horizon / 2.0f - 256));
    m_canvas.fill_rect(geom::frect(0, horizon / 2.0f, size.width() / 2.0f, size.height() / 2.0f),
                       surf::Color(0.3f, 0.35f, 0.55f));
    m_canvas.set_z(-8.0f);
    m_canvas.fill_pattern(m_near_hills, geom::frect(0, horizon / 2.0f - 200, size.width() / 2.0f, horizon / 2.0f + 56),
                          geom::foffset(-m_scroll.x * 0.25f, horizon / 2.0f - 200));
    m_canvas.fill_rect(geom::frect(0, horizon / 2.0f + 56, size.width() / 2.0f, size.height() / 2.0f),
                       surf::Color(0.2f, 0.3f, 0.35f));
    m_canvas.restore();

    geom::frect const visible = m_view.get_clip_rect();

    if (m_use_static) {
      int const chunks_x = map_width / chunk_size;
      for (size_t i = 0; i < m_chunks.size(); ++i) {
        int const cx = static_cast<int>(i) % chunks_x;
        int const cy = static_cast<int>(i) / chunks_x;
        geom::frect const bounds(static_cast<float>(cx * chunk_size * tile_size),
                                 static_cast<float>(cy * chunk_size * tile_size),
                                 static_cast<float>((cx + 1) * chunk_size * tile_size),
                                 static_cast<float>((cy + 1) * chunk_size * tile_size));
        if (geom::intersects(bounds, visible) && !m_chunks[i]->empty()) {
          m_canvas.draw(m_chunks[i].get());
        }
      }
    } else if (m_cull) {
      draw_tiles(m_canvas,
                 static_cast<int>(std::floor(visible.left() / tile_size)),
                 static_cast<int>(std::floor(visible.top() / tile_size)),
                 static_cast<int>(std::ceil(visible.right() / tile_size)),
                 static_cast<int>(std::ceil(visible.bottom() / tile_size)));
    } else {
      draw_tiles(m_canvas, 0, 0, map_width, map_height);
    }

    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.55f, 0.75f, 0.95f),
                                         .world_matrix = m_view.get_matrix()});
  }

private:
  Canvas m_canvas;
  View m_view;
  std::unique_ptr<SurfaceManager> m_surfaces;
  std::vector<Surface> m_tiles;
  Unique<Texture> m_far_hills_texture;
  Surface m_far_hills;
  Unique<Texture> m_near_hills_texture;
  Surface m_near_hills;
  std::vector<int> m_map;
  std::vector<Unique<StaticBatch>> m_chunks;
  bool m_use_static;
  bool m_cull;
  glm::vec2 m_scroll;
  float m_camera_y = 0.0f;
};

WST_DEMO_MAIN(TilemapDemo)

/* EOF */
