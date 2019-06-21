// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "display/framebuffer.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <math.h>

#include <glm/ext.hpp>

#include <logmich/log.hpp>

#include "display/shader.hpp"
#include "math/rgb.hpp"
#include "math/rgba.hpp"
#include "math/rect.hpp"
#include "util/raise_exception.hpp"

Size Framebuffer::size;
GLuint Framebuffer::s_textured_prg = 0;
GLuint Framebuffer::s_flatcolor_prg = 0;
glm::mat4 Framebuffer::s_projection;
glm::mat4 Framebuffer::s_modelview;

namespace {

void print_gl_string(const char* prefix, GLenum name)
{
  const GLubyte* ret = glGetString(name);
  if (ret == nullptr)
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
#ifndef HAVE_OPENGLES2
  // GLEW has issues with OpenGL3.3Core, thus we set
  // glewExperimental=true and eat an error wih glGetError()
  // see https://www.opengl.org/wiki/OpenGL_Loading_Library
  glewExperimental = true;
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
    std::ostringstream str;
    str << "Framebuffer::init(): " << glewGetErrorString(err) << std::endl;
    raise_runtime_error(str.str());
  }
  // eat up an "invalid enum" error produced by GLEW
  glGetError();
#endif

  print_gl_string("GL_VENDOR", GL_VENDOR);
  print_gl_string("GL_RENDERER", GL_RENDERER);
  print_gl_string("GL_VERSION", GL_VERSION);
  print_gl_string("GL_SHADING_LANGUAGE_VERSION", GL_SHADING_LANGUAGE_VERSION);
  // print_gl_string("GL_EXTENSIONS", GL_EXTENSIONS);

  assert_gl("Framebuffer::init()");

  // FIXME: dirty, those never get deleted or anything
  s_textured_prg = create_program("src/shader/textured.vert",
                                  "src/shader/textured.frag");
  s_flatcolor_prg = create_program("src/shader/flatcolor.vert",
                                   "src/shader/flatcolor.frag");

#ifndef HAVE_OPENGLES2
  // FIXME: Dirty!
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
#endif
}

void
Framebuffer::begin_render()
{
  s_projection = glm::ortho(0.0f, static_cast<float>(size.width),
                            static_cast<float>(size.height), 0.0f,
                            1000.0f, -1000.0f);
  assert_gl("Framebuffer::begin_render");
}

void
Framebuffer::end_render()
{
  assert_gl("Framebuffer::end_render");
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
  assert_gl("Framebuffer::draw_rect enter");

  const std::array<float, 2*4> positions = {{
      rect.left, rect.top,
      rect.right, rect.top,
      rect.right, rect.bottom,
      rect.left, rect.bottom,
    }};

  GLuint positions_vbo;
  glGenBuffers(1, &positions_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

  {
    GLint color_loc = get_uniform_location(s_flatcolor_prg, "color");

    glUseProgram(s_flatcolor_prg);
    glUniform4f(color_loc, rgb.r_f(), rgb.g_f(), rgb.b_f(), 1.0f);

    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "projection"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "modelview"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

    GLint position_loc = get_attrib_location(Framebuffer::s_flatcolor_prg, "position");
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glDisableVertexAttribArray(position_loc);

    glUseProgram(0);
  }

  glDeleteBuffers(1, &positions_vbo);

  assert_gl("Framebuffer::draw_rect leave");
}

void
Framebuffer::fill_rect(const Rectf& rect, const RGB& rgb)
{
  assert_gl("Framebuffer::fill_rect enter");

  std::array<float, 2*4> positions = {{
      rect.left, rect.top,
      rect.right, rect.top,
      rect.right, rect.bottom,
      rect.left, rect.bottom,
    }};

  GLuint positions_vbo;
  glGenBuffers(1, &positions_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

  {
    GLint color_loc = get_uniform_location(s_flatcolor_prg, "color");
    glUseProgram(s_flatcolor_prg);
    glUniform4f(color_loc, rgb.r_f(), rgb.g_f(), rgb.b_f(), 1.0f);
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "projection"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "modelview"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

    GLint position_loc = get_attrib_location(Framebuffer::s_flatcolor_prg, "position");
    glEnableVertexAttribArray(position_loc);
    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glDisableVertexAttribArray(position_loc);

    glUseProgram(0);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDeleteBuffers(1, &positions_vbo);

  assert_gl("Framebuffer::fill_rect leave");
}

void
Framebuffer::draw_grid(const Vector2f& offset, const Sizef& size_, const RGBA& rgba)
{
  assert_gl("Framebuffer::draw_grid enter");

  std::vector<float> positions;

  float start_x = fmodf(offset.x, size_.width);
  float start_y = fmodf(offset.y, size_.height);

  for(float x = start_x; x < static_cast<float>(Framebuffer::get_width()); x += size_.width)
  {
    positions.push_back(x);
    positions.push_back(0);

    positions.push_back(x);
    positions.push_back(static_cast<float>(Framebuffer::get_height()));
  }

  for(float y = start_y; y < static_cast<float>(Framebuffer::get_height()); y += size_.height)
  {
    positions.push_back(0);
    positions.push_back(y);

    positions.push_back(static_cast<float>(Framebuffer::get_width()));
    positions.push_back(y);
  }


  GLuint positions_vbo;
  glGenBuffers(1, &positions_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

  {
    GLint color_loc = get_uniform_location(s_flatcolor_prg, "color");
    glUseProgram(s_flatcolor_prg);

    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "projection"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
    glUniformMatrix4fv(get_uniform_location(Framebuffer::s_flatcolor_prg, "modelview"),
                       1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

    glUniform4f(color_loc, rgba.r_f(), rgba.g_f(), rgba.b_f(), rgba.a_f());

    GLint position_loc = get_attrib_location(Framebuffer::s_flatcolor_prg, "position");
    glEnableVertexAttribArray(position_loc);

    glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glDrawArrays(GL_LINES, 0, static_cast<int>(positions.size()) / 2);

    glDisableVertexAttribArray(position_loc);

    glUseProgram(0);
  }

  glDeleteBuffers(1, &positions_vbo);

  assert_gl("Framebuffer::draw_grid leave");
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

SoftwareSurface
Framebuffer::screenshot()
{
  PixelData dst(PixelData::RGB_FORMAT, get_size());
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, dst.get_width(), dst.get_height(),
               GL_RGB, GL_UNSIGNED_BYTE, dst.get_data());
  return SoftwareSurface(std::move(dst)).vflip();
}

void
Framebuffer::set_modelview(const glm::mat4& modelview)
{
  s_modelview = modelview;
}

void
Framebuffer::apply_gamma_ramp(float contrast, float brightness, float gamma)
{
  log_error("not implemented: %1% %2% %3%", contrast, brightness, gamma);
}

/* EOF */
