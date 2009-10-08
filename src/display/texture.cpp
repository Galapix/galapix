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

#include "display/texture.hpp"

#include <GL/glew.h>

#include "math/rect.hpp"
#include "display/framebuffer.hpp"

class TextureImpl
{
public:
  GLuint handle;
  Size   size;

  TextureImpl(const SoftwareSurfaceHandle& src, const Rect& srcrect) :
    handle(),
    size(srcrect.get_size())
  {
    assert(src);

    glGenTextures(1, &handle); 
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, handle);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);

    glPixelStorei(GL_UNPACK_ALIGNMENT,  1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src->get_width());
    
    int gl_format = GL_RGB;
    switch(src->get_format())
      {
        case SoftwareSurface::RGB_FORMAT:
          gl_format = GL_RGB;
          break;

        case SoftwareSurface::RGBA_FORMAT:
          gl_format = GL_RGBA;
          break;

        default:
          assert(!"Texture: Not supposed to be reached");
      }

    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, gl_format,
                 size.width, size.height,
                 0, /* border */
                 gl_format,
                 GL_UNSIGNED_BYTE,
                 src->get_data() + (src->get_pitch() * srcrect.top) + (srcrect.left * src->get_bytes_per_pixel()));

    assert_gl("packing image texture");
    
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S,     GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T,     GL_CLAMP);

    assert_gl("setting texture parameters");
  }

  ~TextureImpl()
  {
    glDeleteTextures(1, &handle);    
  }
};

TextureHandle
Texture::create(const SoftwareSurfaceHandle& src, const Rect& srcrect)
{
  return TextureHandle(new Texture(src, srcrect));
}

Texture::Texture(const SoftwareSurfaceHandle& src, const Rect& srcrect) :
  impl(new TextureImpl(src, srcrect))
{
}

void
Texture::bind()
{
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, impl->handle);
}

int
Texture::get_width() const
{
  return impl->size.width;
}

int
Texture::get_height() const
{
  return impl->size.height;
}

/* EOF */
