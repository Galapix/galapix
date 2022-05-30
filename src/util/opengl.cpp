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

#include "util/opengl.hpp"

#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <iostream>

namespace galapix {

namespace {

char const* gl_error_string(GLenum err)
{
#ifndef HAVE_OPENGLES2
  return reinterpret_cast<char const*>(gluErrorString(err));
#else
  switch(err)
  {
    case GL_NO_ERROR:
      return "GL_NO_ERROR";

    case GL_INVALID_ENUM :
      return "GL_INVALID_ENUM";
      break;

    case GL_INVALID_VALUE :
      return "GL_INVALID_VALUE";
      break;

    case GL_INVALID_OPERATION :
      return "GL_INVALID_OPERATION";
      break;

    case GL_INVALID_FRAMEBUFFER_OPERATION :
      return "GL_INVALID_FRAMEBUFFER_OPERATION";
      break;

    case GL_OUT_OF_MEMORY :
      return "GL_OUT_OF_MEMORY";
      break;

    default:
      return"unknown OpenGL error";
      break;
  };
#endif
}

} // namespace

void assert_gl_helper(char const* filename, int line, char const* message)
{
  GLenum error = glGetError();
  if(error != GL_NO_ERROR)
  {
    std::cerr << filename << ":" << line << ": assert_gl(): OpenGLError while '" << message << "': "
              << gl_error_string(error) << std::endl;
    abort();
  }
}

} // namespace galapix

/* EOF */
