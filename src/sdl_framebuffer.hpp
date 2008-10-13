/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_SDL_FRAMEBUFFER_HPP
#define HEADER_SDL_FRAMEBUFFER_HPP

#include "SDL.h"
#include "math/size.hpp"

/** SDL specific code to init the display */
class SDLFramebuffer
{
private:
  static SDL_Surface* screen;
  static Uint32 flags;
  static Size desktop_resolution;
  static Size window_resolution;

public:
    static void set_video_mode(const Size& size, bool fullscreen, int anti_aliasing);

  static void toggle_fullscreen();

  static int  get_width()  { return screen->w; }
  static int  get_height() { return screen->h; }
  static Size get_size()   { return Size(screen->w, screen->h); }

  static void reshape(const Size& size);
  static void flip();

};

#endif

/* EOF */
