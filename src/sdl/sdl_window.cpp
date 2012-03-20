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

#include <stdexcept>

#include "sdl/sdl_framebuffer.hpp"
#include "util/raise_exception.hpp"

SDLWindow::SDLWindow(const Size& geometry, bool fullscreen, int  anti_aliasing) :
  m_geometry(geometry),
  m_fullscreen(fullscreen),
  m_anti_aliasing(anti_aliasing),
  m_joysticks()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0)
  {
    raise_runtime_error("unable to initialize SDL: " << SDL_GetError());
  }
  else
  {
    atexit(SDL_Quit); 

    int num_joysticks = SDL_NumJoysticks();
    for(int i = 0; i < num_joysticks; ++i)
    {
      SDL_Joystick* joy = SDL_JoystickOpen(i);
      if (joy)
      {
        m_joysticks.push_back(joy);
      }
    }

    SDLFramebuffer::set_video_mode(geometry, fullscreen, anti_aliasing);
  }
}

SDLWindow::~SDLWindow()
{
  for(auto joy: m_joysticks)
  {
    SDL_JoystickClose(joy);
  }

  SDL_Quit();
}

void
SDLWindow::flip()
{
  SDL_GL_SwapBuffers();
}

/* EOF */
