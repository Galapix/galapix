// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// Shows .sprite files of the SuperTux, Windstille and Pingus formats,
// cycling through their actions and showing frame events as they fire.
//
//   --path DIR    load all .sprite files below DIR (default: demo data)
//   --check       only load everything and print a report
//   left/right    previous/next page

#include <algorithm>
#include <format>
#include <iostream>
#include <map>

#include <prio/reader_document.hpp>
#include <surf/palette.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/surface_manager.hpp>
#include <wstsprite/anim_state.hpp>
#include <wstsprite/sprite_data.hpp>
#include <wstsprite/sprite_manager.hpp>

#include "demo_app.hpp"

using namespace wstdisplay;
using namespace wstsprite;

namespace {

struct Entry
{
  std::filesystem::path path;
  SpriteId sprite;
  AnimState state;
  float action_time;
};

struct Popup
{
  std::string text;
  geom::fpoint pos;
  float age;
};

/** Event data, parsed once while loading, see EventRegistry */
struct SoundEvent
{
  std::string name;
  float volume;
};

struct ParticleEvent
{
  std::string kind;
  int count;
};

} // namespace

class SpriteViewerDemo : public wstdemo::DemoApp
{
public:
  SpriteViewerDemo(int argc, char** argv) :
    DemoApp("Sprite Viewer", argc, argv),
    m_path(),
    m_check(false),
    m_events(),
    m_canvas(),
    m_surfaces(),
    m_sprites(),
    m_entries(),
    m_popups(),
    m_page(0),
    m_errors(0)
  {}

  bool parse_option(std::string_view arg, std::function<std::string_view()> const& next) override
  {
    if (arg == "--path") {
      m_path = next();
    } else if (arg == "--check") {
      m_check = true;
    } else {
      return false;
    }
    return true;
  }

  std::string get_usage() const override
  {
    return "  --path DIR           load all .sprite files below DIR\n"
           "  --check              load everything, print a report and exit\n";
  }

  void init() override
  {
    if (m_path.empty()) {
      m_path = get_data("sprites");
    }

    m_events.add("sound", [](Properties const& props) {
      return SoundEvent{props.get_string("name").value_or("?"), props.get_float("volume").value_or(1.0f)};
    });
    m_events.add("particles", [](Properties const& props) {
      return ParticleEvent{props.get_string("kind").value_or("?"), props.get_int("count").value_or(1)};
    });

    m_surfaces = std::make_unique<SurfaceManager>(get_device(), SurfaceManager::Options{
        .params = TextureParams{TextureFilter::Nearest}});

    // games keep running with missing images, show them as magenta squares
    m_surfaces->set_fallback(m_surfaces->create(
      surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, geom::isize(16, 16), surf::palette::magenta)));
    m_sprites = std::make_unique<SpriteManager>(*m_surfaces, &m_events);

    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_directory(m_path)) {
      for (auto const& entry : std::filesystem::recursive_directory_iterator(m_path)) {
        if (entry.path().extension() == ".sprite") {
          files.push_back(entry.path());
        }
      }
    } else {
      files.push_back(m_path);
    }
    std::sort(files.begin(), files.end());

    std::map<std::string, std::pair<int, int>> formats; // ok, failed
    for (auto const& file : files) {
      std::string format = "?";
      try {
        format = prio::ReaderDocument::from_file(file).get_name();
        m_entries.push_back(Entry{file, m_sprites->load(file), {}, 0.0f});
        formats[format].first += 1;
      } catch (std::exception const& err) {
        formats[format].second += 1;
        m_errors += 1;
        std::cerr << "error: " << err.what() << "\n";
      }
    }

    if (m_check) {
      std::cout << std::format("{}: {} sprite files\n", m_path.string(), files.size());
      for (auto const& [format, counts] : formats) {
        std::cout << std::format("  {:16} {:4} loaded, {:3} failed\n", format, counts.first, counts.second);
      }
      int actions = 0;
      int frames = 0;
      for (auto const& e : m_entries) {
        SpriteData const& data = m_sprites->get(e.sprite);
        actions += static_cast<int>(data.get_actions().size());
        for (auto const& a : data.get_actions()) {
          frames += static_cast<int>(a.frames.size());
        }
      }
      std::cout << std::format("  {} actions, {} frames, {} atlas pages\n",
                               actions, frames, m_surfaces->get_packer().get_pages().size());
      get_system().quit();
    }
  }

  int page_size() const { return 24; }
  int page_count() const { return std::max(1, (static_cast<int>(m_entries.size()) + page_size() - 1) / page_size()); }

  void on_event(SDL_Event const& event) override
  {
    if (event.type == SDL_KEYDOWN) {
      if (event.key.keysym.sym == SDLK_RIGHT) {
        m_page = std::min(m_page + 1, page_count() - 1);
      } else if (event.key.keysym.sym == SDLK_LEFT) {
        m_page = std::max(m_page - 1, 0);
      }
    }
  }

  geom::fpoint cell_center(int i) const
  {
    geom::fsize const size(get_size());
    int const columns = 6;
    float const cw = size.width() / static_cast<float>(columns);
    float const ch = (size.height() - 40.0f) / 4.0f;
    return geom::fpoint(cw * (static_cast<float>(i % columns) + 0.5f),
                        40.0f + ch * (static_cast<float>(i / columns) + 0.45f));
  }

  void update(float delta) override
  {
    std::vector<EventRef> events;
    int const first = m_page * page_size();
    for (int i = first; i < std::min(first + page_size(), static_cast<int>(m_entries.size())); ++i) {
      Entry& e = m_entries[static_cast<size_t>(i)];
      SpriteData const& data = m_sprites->get(e.sprite);

      // cycle through the actions
      e.action_time += delta;
      Action const& action = current_action(e.state, data);
      float const show_time = std::max(2.0f, action.get_total_duration());
      if (e.action_time > show_time) {
        e.action_time = 0.0f;
        set_action(e.state, (e.state.action + 1) % static_cast<int>(data.get_actions().size()));
        events.clear();
        fire_current_events(e.state, data, events);
      } else {
        events.clear();
      }

      advance(e.state, data, delta, &events);
      if (e.state.finished) {
        restart(e.state);
      }

      for (EventRef const ref : events) {
        // registered events come with their data already parsed
        FrameEvent const& event = data.get_event(ref);
        std::string text = event.name;
        if (SoundEvent const* sound = event.get<SoundEvent>()) {
          text = std::format("sound: {} ({:.0f}%)", sound->name, sound->volume * 100.0f);
        } else if (ParticleEvent const* particles = event.get<ParticleEvent>()) {
          text = std::format("particles: {} x{}", particles->kind, particles->count);
        }
        m_popups.push_back(Popup{text, cell_center(i - first), 0.0f});
      }
    }

    for (auto& p : m_popups) {
      p.age += delta;
    }
    std::erase_if(m_popups, [](Popup const& p) { return p.age > 1.0f; });
  }

  std::string get_info() const override
  {
    return std::format("{} sprites ({} failed), page {}/{}, left/right to flip pages",
                       m_entries.size(), m_errors, m_page + 1, page_count());
  }

  void draw(Renderer& renderer) override
  {
    m_canvas.clear();

    int const first = m_page * page_size();
    for (int i = first; i < std::min(first + page_size(), static_cast<int>(m_entries.size())); ++i) {
      Entry const& e = m_entries[static_cast<size_t>(i)];
      SpriteData const& data = m_sprites->get(e.sprite);
      geom::fpoint const center = cell_center(i - first);

      // the origin of the sprite
      m_canvas.draw_line(geom::fpoint(center.x() - 40, center.y()), geom::fpoint(center.x() + 40, center.y()),
                         surf::Color(1.0f, 1.0f, 1.0f, 0.2f));
      m_canvas.draw_line(geom::fpoint(center.x(), center.y() - 40), geom::fpoint(center.x(), center.y() + 10),
                         surf::Color(1.0f, 1.0f, 1.0f, 0.2f));

      // keep large sprites inside their cell
      Action const& action = current_action(e.state, data);
      float const h = action.frames.empty() ? 1.0f : action.frames[0].surface.get_height() * action.scale;
      float const fit = std::min(1.0f, 60.0f / h);
      wstsprite::draw(m_canvas, data, e.state, DrawParams().set_pos(center).set_scale(fit));

      if (action.hitbox) {
        geom::frect const hb = *action.hitbox;
        geom::fpoint const tl(center.x() + (hb.left() - action.origin.x()) * fit,
                              center.y() + (hb.top() - action.origin.y()) * fit);
        m_canvas.draw_rect(geom::frect(tl, geom::fsize(hb.width() * fit, hb.height() * fit)),
                           surf::Color(1.0f, 0.3f, 0.3f, 0.5f));
      }

      m_canvas.draw_text(get_font(), geom::fpoint(center.x(), center.y() + 75),
                         e.path.stem().string(), surf::palette::white, TextAlign::Center);
      m_canvas.draw_text(get_font(), geom::fpoint(center.x(), center.y() + 93),
                         std::format("{} [{}/{}]", action.name, e.state.frame + 1, action.frames.size()),
                         surf::palette::lightgray, TextAlign::Center);
    }

    m_canvas.set_z(1.0f);
    for (auto const& p : m_popups) {
      m_canvas.draw_text(get_font(), geom::fpoint(p.pos.x(), p.pos.y() - 60 - 40 * p.age), p.text,
                         surf::Color(1.0f, 0.9f, 0.3f, 1.0f - p.age), TextAlign::Center);
    }

    renderer.render(m_canvas, RenderPass{.clear = surf::Color(0.2f, 0.22f, 0.25f)});
  }

private:
  std::filesystem::path m_path;
  bool m_check;
  EventRegistry m_events;
  Canvas m_canvas;
  std::unique_ptr<SurfaceManager> m_surfaces;
  std::unique_ptr<SpriteManager> m_sprites;
  std::vector<Entry> m_entries;
  std::vector<Popup> m_popups;
  int m_page;
  int m_errors;
};

WST_DEMO_MAIN(SpriteViewerDemo)

/* EOF */
