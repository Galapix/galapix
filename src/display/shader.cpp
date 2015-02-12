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

#include "display/shader.hpp"

#include <fstream>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sstream>

namespace {

void check_gl_error()
{
  GLenum err = glGetError();
  switch(err)
  {
    case GL_NO_ERROR:
      // ok
      break;

    case GL_INVALID_ENUM :
      std::cerr << "OpenGL error: GL_INVALID_ENUM" << std::endl;
      break;

    case GL_INVALID_VALUE :
      std::cerr << "OpenGL error: GL_INVALID_VALUE" << std::endl;
      break;

    case GL_INVALID_OPERATION :
      std::cerr << "OpenGL error: GL_INVALID_OPERATION" << std::endl;
      break;

    case GL_INVALID_FRAMEBUFFER_OPERATION :
      std::cerr << "OpenGL error: GL_INVALID_FRAMEBUFFER_OPERATION" << std::endl;
      break;

    case GL_OUT_OF_MEMORY :
      std::cerr << "OpenGL error: GL_OUT_OF_MEMORY" << std::endl;
      break;

    default:
      std::cerr << "unknown OpenGL error" << std::endl;
      break;
  };
}

std::string read_text_file(const std::string& filename)
{
  std::ifstream fin(filename);
  if (!fin)
  {
    int err = errno;
    std::cerr << "failed to open: " << filename << std::endl;
    std::cerr << strerror(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  {
    std::ostringstream buffer;
    buffer << fin.rdbuf();
    return buffer.str();
  }
}

GLuint compile_shader(GLenum shader_type, const std::string& filename)
{
  std::string source = read_text_file(filename);
  
  std::cout << ">>> compiling '" << filename << "':\n" << source << std::endl;
  
  GLuint shader = glCreateShader(shader_type);
  check_gl_error();
  const GLchar* source_lst[] = { source.c_str() };
  GLint length_lst[] = { static_cast<GLint>(source.size()) };
  glShaderSource(shader, 1, source_lst, length_lst);
  check_gl_error();
  glCompileShader(shader);
  check_gl_error();
  
  GLint status = 45454545;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  check_gl_error();
  if (!status)
  {
    GLchar infolog[4096];
    GLsizei len;
    glGetShaderInfoLog(shader, sizeof(infolog), &len, infolog);
    std::cerr << "Compile error in '" << filename << "': status = " << status << std::endl;
    std::cerr.write(infolog, len);
    exit(EXIT_FAILURE);
  }
  else
  {
    std::cout << ">>> compiling ok: status = " << status << std::endl;
    return shader;
  }
}

} // namespace

GLuint create_program(const std::string& vert_shader_filename,
                      const std::string& frag_shader_filename)
{
  GLuint vert_shader = compile_shader(GL_VERTEX_SHADER, vert_shader_filename);
  GLuint frag_shader = compile_shader(GL_FRAGMENT_SHADER, frag_shader_filename);

  GLuint program = glCreateProgram();

  glAttachShader(program, frag_shader);
  glAttachShader(program, vert_shader);

  glLinkProgram(program);

  glDeleteShader(frag_shader);
  glDeleteShader(vert_shader);
  
  GLint link_status = 45454545;
  glGetProgramiv(program, GL_LINK_STATUS, &link_status);
  check_gl_error();
  if (!link_status)
  {
    GLchar infolog[4096];
    GLsizei len;
    glGetProgramInfoLog(program, sizeof(infolog), &len, infolog);
    std::cerr << "Link error: status = " << link_status << std::endl;
    std::cerr.write(infolog, len);
    exit(EXIT_FAILURE);
  }
  else
  {
    std::cout << "link ok: status = " << link_status << std::endl;
  }
  
  return program;
}

GLint get_uniform_location(GLuint program, const char* name)
{
  GLint loc = glGetUniformLocation(program, name);
  if (loc == -1)
  {
    std::cerr << "Could not get uniform location '" << name << "'" << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  {
    return loc;
  }
}

GLint get_attrib_location(GLuint program, const char* name)
{
  GLint loc = glGetAttribLocation(program, name);
  if (loc == -1)
  {
    std::cerr << "Could not get attrib location '" << name << "'" << std::endl;
    exit(EXIT_FAILURE);
  }
  else
  {
    return loc;
  }
}

/* EOF */
