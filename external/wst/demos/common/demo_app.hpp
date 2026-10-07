// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WST_DEMOS_DEMO_APP_HPP
#define HEADER_WST_DEMOS_DEMO_APP_HPP

#include <filesystem>
#include <functional>
#include <string_view>
#include <vector>
#include <memory>
#include <string>

#include <SDL.h>

#include <geom/size.hpp>
#include <wstdisplay/canvas.hpp>
#include <wstdisplay/device.hpp>
#include <wstdisplay/font/font.hpp>
#include <wstdisplay/font/font_manager.hpp>
#include <wstdisplay/opengl_window.hpp>
#include <wstdisplay/renderer.hpp>
#include <wstsystem/system.hpp>

namespace wstdemo {

/** Common skeleton of the demo applications.

    Command line options:
      --gl / --gles        force GL 3.3 core or GLES 2.0
      --size WxH           window size
      --fullscreen
      --no-vsync
      --hidden             don't show the window
      --frames N           quit after N frames, the simulation then
                           runs with a fixed 1/60s step
      --screenshot FILE    save the last frame to FILE
      --benchmark          print render stats every second
      --datadir DIR        location of the demo data
      --verbose            log OpenGL details

    Keys: Escape quits, F1 toggles the overlay, F11 fullscreen,
    F12 saves a screenshot. */
class DemoApp
{
public:
  DemoApp(std::string title, int argc, char** argv);
  virtual ~DemoApp();

  /** Run the main loop, returns the process exit code */
  int run();

protected:
  /** Handle a demo specific command line option, \a next returns
      the option argument. Return false for unknown options. */
  virtual bool parse_option(std::string_view arg, std::function<std::string_view()> const& next)
  {
    (void)arg; (void)next;
    return false;
  }

  /** Extra lines for --help */
  virtual std::string get_usage() const { return {}; }

  /** Called once the window and renderer exist */
  virtual void init() {}

  virtual void update(float delta) { (void)delta; }

  /** Draw a frame, begin_frame() has already been called */
  virtual void draw(wstdisplay::Renderer& renderer) = 0;

  /** Extra lines for the overlay */
  virtual std::string get_info() const { return {}; }

  virtual void on_event(SDL_Event const& event) { (void)event; }
  virtual void on_resize(geom::isize const& size) { (void)size; }

  wstsystem::System& get_system() { return *m_system; }
  wstdisplay::OpenGLWindow& get_window() { return *m_window; }
  wstdisplay::Device& get_device() { return m_window->get_device(); }
  wstdisplay::Renderer& get_renderer() { return m_window->get_renderer(); }
  geom::isize get_size() const;

  std::filesystem::path get_data(std::filesystem::path const& filename) const;

  wstdisplay::FontManager& get_font_manager() { return *m_font_manager; }
  wstdisplay::Font const& get_font() const { return m_font_manager->get(m_font); }

  /** Seconds since start, deterministic with --frames */
  float get_time() const { return m_time; }
  int get_frame() const { return m_frame; }

  bool is_benchmark() const { return m_benchmark; }

private:
  void parse_args();
  void draw_overlay(wstdisplay::Renderer& renderer);
  void save_screenshot(std::filesystem::path const& filename);

private:
  std::string m_title;
  std::vector<std::string> m_args;

  wstdisplay::OpenGLWindow::Params m_params;
  int m_max_frames;
  std::filesystem::path m_screenshot;
  bool m_benchmark;
  std::filesystem::path m_datadir;

  std::unique_ptr<wstsystem::System> m_system;
  std::unique_ptr<wstdisplay::OpenGLWindow> m_window;
  std::unique_ptr<wstdisplay::FontManager> m_font_manager;
  wstdisplay::FontId m_font;
  wstdisplay::Canvas m_overlay;

  bool m_show_overlay;
  bool m_take_screenshot;
  float m_time;
  int m_frame;
  float m_fps;
};

/** Defines main() for a DemoApp subclass */
#define WST_DEMO_MAIN(DemoClass)                \
  int main(int argc, char** argv)               \
  {                                             \
    DemoClass app(argc, argv);                  \
    return app.run();                           \
  }

} // namespace wstdemo

#endif

/* EOF */
