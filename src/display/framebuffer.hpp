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

#ifndef HEADER_GALAPIX_DISPLAY_FRAMEBUFFER_HPP
#define HEADER_GALAPIX_DISPLAY_FRAMEBUFFER_HPP

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "math/size.hpp"
#include "util/software_surface.hpp"

class RGB;
class RGBA;
class Size;
class Sizef;
class Vector2f;
class Rectf;

#ifdef NDEBUG
void assert_gl(const char* message);
#else
#  define assert_gl(x)
#endif

/** Generic OpenGL helper functions */
class Framebuffer
{
private:
  static Size size;

public:
  static void init();
  static void reshape(const Size& size);
  static int  get_width();
  static int  get_height();
  static Size get_size()   { return Size(get_width(), get_height()); }

  static void clear(const RGBA& rgba);
  static void draw_rect(const Rectf& rect, const RGB& rgb);
  static void fill_rect(const Rectf& rect, const RGB& rgb);
  static void draw_grid(int num_cells);
  static void draw_grid(const Vector2f& offset, const Sizef& size, const RGBA& rgba);

  static SoftwareSurface screenshot();
  static void apply_gamma_ramp(float contrast, float brightness, float gamma);
};

#endif

/* EOF */
