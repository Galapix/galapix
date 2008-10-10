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

#ifndef HEADER_DISPLAY_HPP
#define HEADER_DISPLAY_HPP

#include <GL/gl.h>
#include <GL/glu.h>
#include <sstream>
#include <stdexcept>
#include <math.h>
#include "SDL.h"
#include "math/size.hpp"

class RGB;
class RGBA;
class Size;
class Rectf;

static inline void assert_gl(const char* message)
{
  GLenum error = glGetError();
  if(error != GL_NO_ERROR) {
    std::ostringstream msg;
    msg << "OpenGLError while '" << message << "': "
        << gluErrorString(error);
    throw std::runtime_error(msg.str());
  }
}

class Framebuffer
{
private:
  static SDL_Surface* screen;
  static Uint32 flags;
  static Size desktop_resolution;

public:
  static void set_video_mode(const Size& size, bool fullscreen, int anti_aliasing);

  static void toggle_fullscreen();

  static int  get_width()  { return screen->w; }
  static int  get_height() { return screen->h; }
  static Size get_size()   { return Size(screen->w, screen->h); }

  static SDL_Surface* get_screen() { return screen; }
  static void resize(int w, int h);
  static void flip();
  static void clear(const RGBA& rgba);

  static void draw_rect(const Rectf& rect, const RGB& rgb);
  static void fill_rect(const Rectf& rect, const RGB& rgb);
  static void draw_grid(int num_cells);
};

#endif

/* EOF */
