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

#include "display/framebuffer.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <math.h>

#include "math/rgb.hpp"
#include "math/rgba.hpp"
#include "math/rect.hpp"
#include "util/raise_exception.hpp"

Size Framebuffer::size;

#ifndef assert_gl
void assert_gl(const char* message)
{
  GLenum error = glGetError();
  if(error != GL_NO_ERROR) {
    std::ostringstream msg;
    msg << "assert_gl(): OpenGLError while '" << message << "': "
        << gluErrorString(error);
    raise_runtime_error(msg.str());
  }
}
#endif

void 
Framebuffer::init()
{
  // Init Glew 
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    std::ostringstream str;
    str << "Framebuffer::init(): " << glewGetErrorString(err) << std::endl;
    raise_runtime_error(str.str());
  }
  
  if (!GLEW_ARB_texture_rectangle)
  {
    raise_runtime_error("Framebuffer::init(): OpenGL ARB_texture_rectangle extension not found, but required");
  }
}

void
Framebuffer::reshape(const Size& size_)
{ 
  size = size_;

  glViewport(0, 0, size.width, size.height);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, size.width, size.height, 0.0, 1000.0, -1000.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  // Magic Voodoo to get pixel perfect graphics, see:
  //   http://www.opengl.org/resources/faq/technical/transformations.htm
  // On Nvidia this seems to break things, instead of fixing them, so
  // we don't use it, since the results are already perfect even without it.
  // glTranslated(0.375, 0.375, 0.0);
}

void
Framebuffer::clear(const RGBA& rgba)
{
  glClearColor(static_cast<float>(rgba.r)/255.0f,
               static_cast<float>(rgba.g)/255.0f,
               static_cast<float>(rgba.b)/255.0f,
               static_cast<float>(rgba.a)/255.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
Framebuffer::draw_rect(const Rectf& rect, const RGB& rgb)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
    
  glColor3ub(rgb.r, rgb.g, rgb.b);

  glBegin(GL_LINE_LOOP);
  glVertex2f(rect.left,  rect.top);
  glVertex2f(rect.right, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glVertex2f(rect.left,  rect.bottom);
  glEnd();
}

void
Framebuffer::fill_rect(const Rectf& rect, const RGB& rgb)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glColor3ub(rgb.r, rgb.g, rgb.b);
  glBegin(GL_QUADS);
  glVertex2f(rect.left,  rect.top);
  glVertex2f(rect.right, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glVertex2f(rect.left,  rect.bottom);
  glEnd();
}

void
Framebuffer::draw_grid(int num_cells)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
 
  glBegin(GL_LINES);
  //  if (grid_color)
  glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
  //else
  //  glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

  int cell_width = Framebuffer::get_width()/num_cells;
  for(int x = 1; x < num_cells; ++x)
  {
    glVertex2i(x*cell_width, 0);
    glVertex2i(x*cell_width, Framebuffer::get_height());
  }

  int cell_height = Framebuffer::get_height()/num_cells;
  for(int y = 1; y < num_cells; ++y)
  {
    glVertex2i(0, y*cell_height);
    glVertex2i(Framebuffer::get_width(), y*cell_height);
  }

  glEnd();
}

void
Framebuffer::draw_grid(const Vector2f& offset, const Sizef& size_, const RGBA& rgba)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
 
  glBegin(GL_LINES);
  glColor4ub(rgba.r, rgba.g, rgba.b, rgba.a);

  float start_x = fmodf(offset.x, size_.width);
  float start_y = fmodf(offset.y, size_.height);

  for(float x = start_x; x < Framebuffer::get_width(); x += size_.width)
  {
    glVertex2f(x, 0);
    glVertex2f(x, static_cast<float>(Framebuffer::get_height()));
  }

  for(float y = start_y; y < Framebuffer::get_height(); y += size_.height)
  {
    glVertex2f(0, y);
    glVertex2f(static_cast<float>(Framebuffer::get_width()), y);
  }

  glEnd();  
}

int
Framebuffer::get_width()
{
  return size.width;
}

int
Framebuffer::get_height()
{
  return size.height;
}

SoftwareSurfacePtr
Framebuffer::screenshot()
{
  SoftwareSurfacePtr surface = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT, get_size());
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, surface->get_width(), surface->get_height(),
               GL_RGB, GL_UNSIGNED_BYTE, surface->get_data());
  return surface->vflip();
}

void
Framebuffer::apply_gamma_ramp(float contrast, float brightness, float gamma)
{
#ifdef GALAPIX_SDL
  assert(!"not implemented");
  //SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

/* EOF */
