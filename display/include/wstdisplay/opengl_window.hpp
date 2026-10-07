// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_OPENGL_WINDOW_HPP
#define HEADER_WSTDISPLAY_OPENGL_WINDOW_HPP

#include <filesystem>
#include <memory>
#include <string>

#include <wstsystem/signal.hpp>
#include <SDL.h>

#include <geom/size.hpp>
#include <surf/software_surface.hpp>
#include <wstsystem/fwd.hpp>

#include "fwd.hpp"

namespace wstdisplay {

/** An SDL window with an OpenGL context, its Device and a Renderer */
class OpenGLWindow final
{
public:
  enum class Mode { Fullscreen, FullscreenDesktop, Window };

  enum class Api
  {
    /** GL 3.3 core, falling back to GLES 2.0. GLES 2.0 only on
        Android and Emscripten. */
    Auto,
    GL33,
    GLES2
  };

  struct Params
  {
    std::string title = {};
    std::filesystem::path icon = {};
    geom::isize size = geom::isize(640, 480);
    Mode mode = Mode::Window;
    bool resizable = false;
    int anti_aliasing = 0;
    Api api = Api::Auto;
    bool vsync = true;
    /** Create the window hidden, e.g. for offscreen rendering */
    bool hidden = false;
  };

public:
  OpenGLWindow(wstsystem::System& system, Params const& params);
  ~OpenGLWindow();

  void set_title(std::string const& title);
  void set_icon(std::filesystem::path const& filename);

  void set_size(geom::isize const& size);
  geom::isize get_size() const;

  /** Size of the drawable in pixels, differs from get_size() on high-DPI displays */
  geom::isize get_drawable_size() const;

  void set_mode(Mode mode);
  Mode get_mode() const;

  void set_vsync(bool vsync);
  void set_gamma(float r, float g, float b);

  Device& get_device() const;
  Renderer& get_renderer() const;

  void swap_buffers();

  /** Read back the window content, call before swap_buffers() */
  surf::SoftwareSurface screenshot() const;

  uint32_t get_id() const;
  SDL_Window* get_sdl_window() const { return m_window; }

  void handle_event(SDL_WindowEvent const& ev);

public:
  wstsystem::Signal<void(geom::isize const&)> sig_resized;

private:
  bool create(Params const& params, Api api);
  void destroy();

private:
  wstsystem::System& m_system;
  SDL_Window* m_window;
  SDL_GLContext m_gl_context;
  geom::isize m_size;
  Mode m_mode;
  std::unique_ptr<Device> m_device;
  std::unique_ptr<Renderer> m_renderer;

public:
  OpenGLWindow(OpenGLWindow const&) = delete;
  OpenGLWindow& operator=(OpenGLWindow const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
