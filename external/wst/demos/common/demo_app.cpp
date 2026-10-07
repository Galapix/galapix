// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "demo_app.hpp"

#include <chrono>
#include <cstdlib>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include <logmich/log.hpp>
#include <surf/palette.hpp>
#include <surf/save.hpp>
#include <wstdisplay/caps.hpp>

namespace wstdemo {

namespace {

geom::isize parse_size(std::string_view text)
{
  auto const x = text.find('x');
  if (x == std::string_view::npos) {
    throw std::invalid_argument("invalid size: " + std::string(text));
  }
  return geom::isize(std::stoi(std::string(text.substr(0, x))),
                     std::stoi(std::string(text.substr(x + 1))));
}

} // namespace

DemoApp::DemoApp(std::string title, int argc, char** argv) :
  m_title(std::move(title)),
  m_args(argv, argv + argc),
  m_params(),
  m_max_frames(0),
  m_screenshot(),
  m_benchmark(false),
  m_datadir(),
  m_system(),
  m_window(),
  m_font_manager(),
  m_font(),
  m_overlay(),
  m_show_overlay(true),
  m_take_screenshot(false),
  m_time(0.0f),
  m_frame(0),
  m_fps(0.0f)
{
  m_params.title = m_title;
  m_params.size = geom::isize(1280, 720);
  m_params.resizable = true;
}

DemoApp::~DemoApp()
{
}

void
DemoApp::parse_args()
{
  for (size_t i = 1; i < m_args.size(); ++i)
  {
    std::string_view const arg = m_args[i];
    std::function<std::string_view()> const next = [&]() -> std::string_view {
      if (i + 1 >= m_args.size()) {
        throw std::invalid_argument(std::string(arg) + " requires an argument");
      }
      return m_args[++i];
    };

    if (arg == "--gl") {
      m_params.api = wstdisplay::OpenGLWindow::Api::GL33;
    } else if (arg == "--gles") {
      m_params.api = wstdisplay::OpenGLWindow::Api::GLES2;
    } else if (arg == "--size") {
      m_params.size = parse_size(next());
    } else if (arg == "--fullscreen") {
      m_params.mode = wstdisplay::OpenGLWindow::Mode::FullscreenDesktop;
    } else if (arg == "--no-vsync") {
      m_params.vsync = false;
    } else if (arg == "--hidden") {
      m_params.hidden = true;
    } else if (arg == "--frames") {
      m_max_frames = std::stoi(std::string(next()));
    } else if (arg == "--screenshot") {
      m_screenshot = next();
    } else if (arg == "--verbose" || arg == "-v") {
      logmich::set_log_level(logmich::LogLevel::INFO);
    } else if (arg == "--benchmark") {
      m_benchmark = true;
    } else if (arg == "--datadir") {
      m_datadir = next();
    } else if (arg == "--help" || arg == "-h") {
      std::cout << "Usage: " << m_args[0] << " [OPTIONS]\n"
                << "  --gl / --gles        force GL 3.3 core or GLES 2.0\n"
                << "  --size WxH           window size\n"
                << "  --fullscreen\n"
                << "  --no-vsync\n"
                << "  --hidden             don't show the window\n"
                << "  --frames N           quit after N frames (fixed 1/60s steps)\n"
                << "  --screenshot FILE    save the last frame to FILE\n"
                << "  --benchmark          print render stats every second\n"
                << "  --datadir DIR        location of the demo data\n"
                << "  --verbose            log OpenGL details\n"
                << get_usage();
      std::exit(EXIT_SUCCESS);
    } else if (!parse_option(arg, next)) {
      throw std::invalid_argument("unknown option: " + std::string(arg));
    }
  }

  if (m_datadir.empty()) {
    if (char const* env = std::getenv("WST_DEMO_DATADIR")) {
      m_datadir = env;
    } else if (std::filesystem::exists(WST_DEMO_SOURCE_DATADIR)) {
      m_datadir = WST_DEMO_SOURCE_DATADIR;
    } else {
      m_datadir = WST_DEMO_INSTALL_DATADIR;
    }
  }
}

geom::isize
DemoApp::get_size() const
{
  return m_window->get_drawable_size();
}

std::filesystem::path
DemoApp::get_data(std::filesystem::path const& filename) const
{
  return m_datadir / filename;
}

int
DemoApp::run()
{
  try
  {
    parse_args();

    m_system = std::make_unique<wstsystem::System>();
    m_window = m_system->create_window(m_params);
    m_font_manager = std::make_unique<wstdisplay::FontManager>(m_window->get_device());
    m_font = m_font_manager->load(get_data("Vera.ttf"), 14);

    m_system->sig_event().connect([this](SDL_Event const& event) {
      if (event.type == SDL_KEYDOWN && !event.key.repeat) {
        switch (event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
            m_system->quit();
            break;

          case SDLK_F1:
            m_show_overlay = !m_show_overlay;
            break;

          case SDLK_F11:
            m_window->set_mode(m_window->get_mode() == wstdisplay::OpenGLWindow::Mode::Window ?
                               wstdisplay::OpenGLWindow::Mode::FullscreenDesktop :
                               wstdisplay::OpenGLWindow::Mode::Window);
            break;

          case SDLK_F12:
            m_take_screenshot = true;
            break;

          default:
            break;
        }
      }
      on_event(event);
    });

    m_window->sig_resized.connect([this](geom::isize const&) {
      on_resize(get_size());
    });

    init();

    using clock = std::chrono::steady_clock;
    auto const start_time = clock::now();
    auto last_time = start_time;
    auto fps_time = last_time;
    int fps_frames = 0;

    while (!m_system->is_quit())
    {
      m_system->update();

      auto const now = clock::now();
      float delta = std::chrono::duration<float>(now - last_time).count();
      last_time = now;
      if (m_max_frames > 0) {
        delta = 1.0f / 60.0f;
      }
      delta = std::min(delta, 0.1f);

      m_time += delta;
      update(delta);

      wstdisplay::Renderer& renderer = get_renderer();
      renderer.begin_frame(get_size());
      draw(renderer);

      fps_frames += 1;
      float const fps_elapsed = std::chrono::duration<float>(now - fps_time).count();
      if (fps_elapsed >= 1.0f) {
        m_fps = static_cast<float>(fps_frames) / fps_elapsed;
        fps_frames = 0;
        fps_time = now;
        if (m_benchmark) {
          auto const& stats = renderer.get_stats();
          std::cout << std::format("{}: {:.1f} fps, {:.2f} ms/frame, {} draw calls, {} vertices, {} commands\n",
                                   m_title, m_fps, 1000.0f / m_fps, stats.draw_calls, stats.vertices, stats.commands);
        }
      }

      m_frame += 1;
      bool const last_frame = (m_max_frames > 0 && m_frame >= m_max_frames);

      // keep automated screenshots free of the changing overlay
      if (m_show_overlay && m_screenshot.empty()) {
        draw_overlay(renderer);
      }

      if (m_take_screenshot) {
        save_screenshot(std::format("wst-screenshot-{}.png", std::chrono::system_clock::now().time_since_epoch().count()));
        m_take_screenshot = false;
      }

      if (last_frame && !m_screenshot.empty()) {
        save_screenshot(m_screenshot);
      }

      renderer.end_frame();
      m_window->swap_buffers();

      if (last_frame) {
        break;
      }
    }

    if (m_benchmark) {
      float const total = std::chrono::duration<float>(clock::now() - start_time).count();
      std::cout << std::format("{}: average over {} frames: {:.1f} fps, {:.3f} ms/frame\n",
                               m_title, m_frame, static_cast<float>(m_frame) / total,
                               1000.0f * total / static_cast<float>(m_frame));
    }

    // GPU resources of the demo are released before the window, as
    // members of the derived class go away first
    return EXIT_SUCCESS;
  }
  catch (std::exception const& err)
  {
    std::cerr << m_title << ": error: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}

void
DemoApp::draw_overlay(wstdisplay::Renderer& renderer)
{
  auto const stats = renderer.get_stats();
  auto const& caps = renderer.get_caps();

  std::string text = std::format("{}\n{} - {}\n{:.1f} fps  {} draw calls  {} batches  {} commands\n"
                                 "{} vertices  {} triangles  {} texture binds",
                                 m_title, caps.is_gles() ? "GLES 2.0" : "GL 3.3", caps.renderer,
                                 m_fps, stats.draw_calls, stats.batches, stats.commands,
                                 stats.vertices, stats.triangles, stats.texture_binds);
  std::string const info = get_info();
  if (!info.empty()) {
    text += "\n" + info;
  }

  wstdisplay::Font const& font = get_font();
  geom::fsize const size = font.get_size(text);
  m_overlay.clear();
  m_overlay.fill_rect(geom::frect(4, 4, 16 + size.width(), 12 + size.height()), surf::Color(0.0f, 0.0f, 0.0f, 0.6f));
  m_overlay.draw_text(font, geom::fpoint(10, 8 + font.get_height()), text);
  renderer.render(m_overlay);
}

void
DemoApp::save_screenshot(std::filesystem::path const& filename)
{
  surf::save(get_renderer().read_pixels(), filename);
  log_info("screenshot saved to {}", filename.string());
}

} // namespace wstdemo

/* EOF */
