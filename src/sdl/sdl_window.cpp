/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "sdl/sdl_window.hpp"

#include <assert.h>
#include <stdexcept>

#include "display/framebuffer.hpp"
#include "util/raise_exception.hpp"

SDLWindow::SDLWindow(const Size& geometry, bool fullscreen, int  anti_aliasing) :
  m_window(0),
  m_gl_context(0),
  m_geometry(geometry),
  m_fullscreen(fullscreen),
  m_anti_aliasing(anti_aliasing)
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
  {
    raise_runtime_error("unable to initialize SDL: " << SDL_GetError());
  }
  else
  {
    atexit(SDL_Quit);
    set_video_mode(geometry, fullscreen, anti_aliasing);
  }
}

SDLWindow::~SDLWindow()
{
  SDL_GL_DeleteContext(m_gl_context);
  SDL_DestroyWindow(m_window);
}

void
SDLWindow::set_video_mode(const Size& size, bool fullscreen, int anti_aliasing)
{
  assert(m_window == 0);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

  if (anti_aliasing > 0)
  {
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, anti_aliasing);
  }

  //SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  //SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1); // vsync

  Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
  if (fullscreen)
  {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    m_fullscreen = true;
  }
  else
  {
    m_fullscreen = false;
  }

  m_window = SDL_CreateWindow("Galapix 0.3.0",
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              size.width, size.height,
                              flags);
  if (!m_window)
  {
    raise_runtime_error("unable to set video mode: " << SDL_GetError());
  }

  m_gl_context = SDL_GL_CreateContext(m_window);
  if (!m_gl_context)
  {
    throw std::runtime_error("Display:: failed to create GLContext");
  }

  Framebuffer::init();

  Size s;
  SDL_GetWindowSize(m_window, &s.width, &s.height);
  Framebuffer::reshape(s);
}

void
SDLWindow::flip()
{
  SDL_GL_SwapWindow(m_window);
}

void
SDLWindow::toggle_fullscreen()
{
  if (!m_fullscreen)
  {
    SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    m_fullscreen = true;
  }
  else
  {
    SDL_SetWindowFullscreen(m_window, 0);
    m_fullscreen = false;
  }

  Size s;
  SDL_GetWindowSize(m_window, &s.width, &s.height);
  Framebuffer::reshape(s);
}

void
SDLWindow::apply_gamma_ramp(float contrast, float brightness, float gamma)
{
  assert(!"not implemented");
#if 0
  Uint16 tbl[256];
  for(int i = 0; i < 256; ++i)
  {
    float c = static_cast<float>(i)/255.0f;
    c = c + brightness;
    c = (c * contrast) - 0.5f * (contrast - 1.0f);
    c = powf(c, 1.0f/gamma);

    tbl[i] = static_cast<Uint16>(Math::clamp(0, (int)(c*65535.0f), 65535));
  }

  SDL_SetGammaRamp(tbl, tbl, tbl);
#endif
}

/* EOF */
