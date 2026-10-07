// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/opengl_window.hpp>

#include <bit>
#include <stdexcept>

#include <logmich/log.hpp>
#include <surf/software_surface.hpp>

#include <wstdisplay/device.hpp>
#include <wstdisplay/renderer.hpp>
#include <wstsystem/system.hpp>

namespace wstdisplay {

OpenGLWindow::OpenGLWindow(wstsystem::System& system, Params const& params) :
  sig_resized(),
  m_system(system),
  m_window(nullptr),
  m_gl_context(nullptr),
  m_size(params.size),
  m_mode(params.mode),
  m_device(),
  m_renderer()
{
  std::vector<Api> apis;
  switch (params.api)
  {
    case Api::Auto:
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
      apis = { Api::GLES2 };
#else
      apis = { Api::GL33, Api::GLES2 };
#endif
      break;

    case Api::GL33:
      apis = { Api::GL33 };
      break;

    case Api::GLES2:
      apis = { Api::GLES2 };
      break;
  }

  bool created = false;
  for (Api const api : apis) {
    if (create(params, api)) {
      created = true;
      break;
    }
  }

  if (!created) {
    throw std::runtime_error(std::string("OpenGLWindow: couldn't create an OpenGL context: ") + SDL_GetError());
  }

  if (!params.icon.empty()) {
    set_icon(params.icon);
  }
}

bool
OpenGLWindow::create(Params const& params, Api api)
{
  SDL_GL_ResetAttributes();
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  if (params.anti_aliasing != 0) {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, params.anti_aliasing);
  }

  if (api == Api::GLES2) {
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  } else {
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  }

  uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
  if (params.mode == Mode::Fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN;
  } else if (params.mode == Mode::FullscreenDesktop) {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }
  if (params.resizable) {
    flags |= SDL_WINDOW_RESIZABLE;
  }
  if (params.hidden) {
    flags |= SDL_WINDOW_HIDDEN;
  }

  char const* api_name = (api == Api::GLES2) ? "GLES 2.0" : "GL 3.3 core";

  m_window = SDL_CreateWindow(params.title.c_str(),
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              params.size.width(), params.size.height(),
                              flags);
  if (m_window == nullptr) {
    log_warn("OpenGLWindow: couldn't create window for {}: {}", api_name, SDL_GetError());
    return false;
  }

  m_gl_context = SDL_GL_CreateContext(m_window);
  if (m_gl_context == nullptr) {
    log_warn("OpenGLWindow: couldn't create {} context: {}", api_name, SDL_GetError());
    destroy();
    return false;
  }

  set_vsync(params.vsync);

  try {
    m_device = std::make_unique<Device>(&SDL_GL_GetProcAddress);
    m_renderer = std::make_unique<Renderer>(*m_device);
  } catch (std::exception const& err) {
    log_warn("OpenGLWindow: couldn't initialize renderer on {}: {}", api_name, err.what());
    destroy();
    return false;
  }

  m_size = get_size();
  return true;
}

void
OpenGLWindow::destroy()
{
  m_renderer.reset();
  m_device.reset();
  if (m_gl_context != nullptr) {
    SDL_GL_DeleteContext(m_gl_context);
    m_gl_context = nullptr;
  }
  if (m_window != nullptr) {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }
}

OpenGLWindow::~OpenGLWindow()
{
  if (m_window) {
    m_system.remove_window(get_id());
  }
  destroy();
}

void
OpenGLWindow::set_title(std::string const& title)
{
  SDL_SetWindowTitle(m_window, title.c_str());
}

void
OpenGLWindow::set_icon(std::filesystem::path const& filename)
{
  surf::SoftwareSurface const pixeldata = surf::SoftwareSurface::from_file(filename);
  bool const rgb = pixeldata.get_format() == surf::PixelFormat::RGB8;

  // byte order R, G, B, A in memory
  uint32_t rmask, gmask, bmask, amask;
  if constexpr (std::endian::native == std::endian::big) {
    rmask = rgb ? 0xff0000 : 0xff000000;
    gmask = rgb ? 0x00ff00 : 0x00ff0000;
    bmask = rgb ? 0x0000ff : 0x0000ff00;
    amask = rgb ? 0 : 0x000000ff;
  } else {
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = rgb ? 0 : 0xff000000;
  }

  SDL_Surface* sdl_surface = SDL_CreateRGBSurfaceFrom(const_cast<void*>(pixeldata.get_data()),
                                                      pixeldata.get_width(), pixeldata.get_height(),
                                                      rgb ? 24 : 32, pixeldata.get_pitch(),
                                                      rmask, gmask, bmask, amask);
  if (sdl_surface) {
    SDL_SetWindowIcon(m_window, sdl_surface);
    SDL_FreeSurface(sdl_surface);
  }
}

void
OpenGLWindow::set_size(geom::isize const& size)
{
  SDL_SetWindowSize(m_window, size.width(), size.height());
  m_size = get_size();
}

geom::isize
OpenGLWindow::get_size() const
{
  int w, h;
  SDL_GetWindowSize(m_window, &w, &h);
  return geom::isize(w, h);
}

geom::isize
OpenGLWindow::get_drawable_size() const
{
  int w, h;
  SDL_GL_GetDrawableSize(m_window, &w, &h);
  return geom::isize(w, h);
}

void
OpenGLWindow::set_mode(Mode mode)
{
  m_mode = mode;
  switch (m_mode)
  {
    case Mode::FullscreenDesktop:
      SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
      break;

    case Mode::Fullscreen:
      SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
      break;

    case Mode::Window:
      SDL_SetWindowFullscreen(m_window, 0);
      break;
  }
}

OpenGLWindow::Mode
OpenGLWindow::get_mode() const
{
  return m_mode;
}

void
OpenGLWindow::set_vsync(bool vsync)
{
  if (SDL_GL_SetSwapInterval(vsync ? 1 : 0) != 0) {
    log_debug("OpenGLWindow: couldn't set swap interval: {}", SDL_GetError());
  }
}

void
OpenGLWindow::set_gamma(float r, float g, float b)
{
  SDL_SetWindowBrightness(m_window, (r + g + b) / 3.0f);
}

Device&
OpenGLWindow::get_device() const
{
  return *m_device;
}

Renderer&
OpenGLWindow::get_renderer() const
{
  return *m_renderer;
}

void
OpenGLWindow::swap_buffers()
{
  SDL_GL_SwapWindow(m_window);
}

surf::SoftwareSurface
OpenGLWindow::screenshot() const
{
  return m_renderer->read_pixels();
}

uint32_t
OpenGLWindow::get_id() const
{
  return SDL_GetWindowID(m_window);
}

void
OpenGLWindow::handle_event(SDL_WindowEvent const& window)
{
  switch (window.event)
  {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
      m_size = geom::isize(window.data1, window.data2);
      sig_resized(m_size);
      break;

    default:
      break;
  }
}

} // namespace wstdisplay

/* EOF */
