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

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "display/shader.hpp"
#include "math/rgb.hpp"
#include "math/rgba.hpp"
#include "math/rect.hpp"

Size Framebuffer::size;
GLuint Framebuffer::s_texured_prg = 0;
GLuint Framebuffer::s_flatcolor_prg = 0;
glm::mat4 Framebuffer::s_projection;
glm::mat4 Framebuffer::s_modelview;

#ifndef assert_gl
void assert_gl(const char* message)
{
  GLenum error = glGetError();
  if(error != GL_NO_ERROR) {
    std::ostringstream msg;
    msg << "assert_gl(): OpenGLError while '" << message << "': "
        << gluErrorString(error);
    throw std::runtime_error(msg.str());
  }
}
#endif

namespace {

void print_gl_string(const char* prefix, GLenum name)
{
  const GLubyte* ret = glGetString(name);
  if (ret == 0)
  {
    std::cerr << "error getting string: " << prefix << std::endl;
  }
  else
  {
    std::cerr << prefix << ": " << ret << std::endl;
  }
}

} // namespace

void
Framebuffer::init()
{
  // Init Glew
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    std::ostringstream str;
    str << "Framebuffer::init(): " << glewGetErrorString(err) << std::endl;
    throw std::runtime_error(str.str());
  }

  print_gl_string("GL_VENDOR", GL_VENDOR);
  print_gl_string("GL_RENDERER", GL_RENDERER);
  print_gl_string("GL_VERSION", GL_VERSION);
  print_gl_string("GL_SHADING_LANGUAGE_VERSION", GL_SHADING_LANGUAGE_VERSION);
  // print_gl_string("GL_EXTENSIONS", GL_EXTENSIONS);

  // FIXME: dirty, those never get deleted or anything
  s_texured_prg = create_program("src/shader/textured.vert",
                                 "src/shader/textured.frag");
  s_flatcolor_prg = create_program("src/shader/flatcolor.vert",
                                   "src/shader/flatcolor.frag");
}

void
Framebuffer::begin_render()
{
  s_projection = glm::ortho(0.0f, static_cast<float>(size.width),
                            static_cast<float>(size.height), 0.0f,
                            1000.0f, -1000.0f);

  if (false)
  {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(s_projection));

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(s_modelview));
  }
}

void
Framebuffer::end_render()
{

}

void
Framebuffer::reshape(const Size& size_)
{
  size = size_;

  glViewport(0, 0, size.width, size.height);

  s_projection = glm::ortho(0.0f, static_cast<float>(size.width),
                            static_cast<float>(size.height), 0.0f,
                            1000.0f, -1000.0f);
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
  const std::array<float, 2*4> positions = {
    rect.left, rect.top,
    rect.right, rect.top,
    rect.right, rect.bottom,
    rect.left, rect.bottom,
  };

  if (true)
  {
    GLint color_loc = get_uniform_location(s_flatcolor_prg, "color");

    glUseProgram(s_flatcolor_prg);
    glUniform4f(color_loc, rgb.r/255.0f, rgb.g/255.0f, rgb.b/255.0f, 1.0f);

    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "projection"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "modelview"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

    GLint position_loc = get_attrib_location(Framebuffer::s_texured_prg, "position");
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, positions.data());

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glDisableVertexAttribArray(position_loc);
    
    glUseProgram(0);
  }
  else
  {
    glColor3ub(rgb.r, rgb.g, rgb.b);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, positions.data());
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
  }
}

void
Framebuffer::fill_rect(const Rectf& rect, const RGB& rgb)
{
  std::array<float, 2*4> positions = {
    rect.left, rect.top,
    rect.right, rect.top,
    rect.right, rect.bottom,
    rect.left, rect.bottom,
  };

  if (true)
  {
    GLint color_loc = get_uniform_location(s_flatcolor_prg, "color");
    glUseProgram(s_flatcolor_prg);
    glUniform4f(color_loc, rgb.r/255.0f, rgb.g/255.0f, rgb.b/255.0f, 1.0f);

    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "projection"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "modelview"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

    GLint position_loc = get_attrib_location(Framebuffer::s_texured_prg, "position");
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, positions.data());
            
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(position_loc);
    
    glUseProgram(0);
  }
  else
  {
    glEnableClientState(GL_VERTEX_ARRAY);

    glVertexPointer(2, GL_FLOAT, 0, positions.data());
    glColor3ub(rgb.r, rgb.g, rgb.b);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);
  }
}

void
Framebuffer::draw_grid(const Vector2f& offset, const Sizef& size_, const RGBA& rgba)
{
  std::vector<float> positions;

  float start_x = fmodf(offset.x, size_.width);
  float start_y = fmodf(offset.y, size_.height);

  for(float x = start_x; x < Framebuffer::get_width(); x += size_.width)
  {
    positions.push_back(x);
    positions.push_back(0);

    positions.push_back(x);
    positions.push_back(static_cast<float>(Framebuffer::get_height()));
  }

  for(float y = start_y; y < Framebuffer::get_height(); y += size_.height)
  {
    positions.push_back(0);
    positions.push_back(y);

    positions.push_back(static_cast<float>(Framebuffer::get_width()));
    positions.push_back(y);
  }

  if (true)
  {
    GLint color_loc = get_uniform_location(s_flatcolor_prg, "color");
    glUseProgram(s_flatcolor_prg);

    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "projection"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "modelview"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

    glUniform4f(color_loc, rgba.r/255.0f, rgba.g/255.0f, rgba.b/255.0f, rgba.a/255.0f);

    GLint position_loc = get_attrib_location(Framebuffer::s_texured_prg, "position");
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, positions.data());

    glDrawArrays(GL_LINES, 0, positions.size()/2);

    glDisableVertexAttribArray(position_loc);
    
    glUseProgram(0);
  }
  else
  {
    glColor4ub(rgba.r, rgba.g, rgba.b, rgba.a);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, positions.data());
    glDrawArrays(GL_LINES, 0, positions.size()/2);
    glDisableClientState(GL_VERTEX_ARRAY);
  }
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
Framebuffer::set_modelview(const glm::mat4& modelview)
{
  s_modelview = modelview;
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
