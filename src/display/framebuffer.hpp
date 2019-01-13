/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>

#include "math/size.hpp"
#include "util/opengl.hpp"
#include "util/software_surface.hpp"

class RGB;
class RGBA;
class Size;
class Sizef;
class Rectf;

/** Generic OpenGL helper functions */
class Framebuffer
{
public:
  static void init();
  static void reshape(const Size& size);
  static int  get_width();
  static int  get_height();
  static Size get_size()   { return Size(get_width(), get_height()); }

  static void clear(const RGBA& rgba);
  static void draw_rect(const Rectf& rect, const RGB& rgb);
  static void fill_rect(const Rectf& rect, const RGB& rgb);
  static void draw_grid(const Vector2f& offset, const Sizef& size, const RGBA& rgba);

  static void set_modelview(const glm::mat4& modelview);

  static void begin_render();
  static void end_render();

  static SoftwareSurface screenshot();
  static void apply_gamma_ramp(float contrast, float brightness, float gamma);

private:
  static Size size;

public:
  static GLuint s_textured_prg;
  static GLuint s_flatcolor_prg;
  static glm::mat4 s_projection;
  static glm::mat4 s_modelview;
};

#endif

/* EOF */
