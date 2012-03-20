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

#ifndef HEADER_GALAPIX_SDL_SDL_WINDOW_HPP
#define HEADER_GALAPIX_SDL_SDL_WINDOW_HPP

#include <SDL.h>
#include <vector>

#include "math/size.hpp"

class SDLWindow
{
protected:
  Size m_geometry;
  bool m_fullscreen;
  int  m_anti_aliasing;

  std::vector<SDL_Joystick*> m_joysticks;

public:
  SDLWindow(const Size& geometry, bool fullscreen, int  anti_aliasing);
  virtual ~SDLWindow();

  /*
    void set_video_mode(const Size& size, bool fullscreen, int anti_aliasing);

    void toggle_fullscreen();

    int  get_width()  { return screen->w; }
    int  get_height() { return screen->h; }
    Size get_size()   { return Size(screen->w, screen->h); }

    void reshape(const Size& size);
    void apply_gamma_ramp(float contrast, float brightness, float gamma);
  */
    void flip();

private:
  SDLWindow(const SDLWindow&) = delete;
  SDLWindow& operator=(const SDLWindow&) = delete;
};

#endif

/* EOF */
