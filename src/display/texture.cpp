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

#include "display/framebuffer.hpp"
#include "math/rect.hpp"
#include "util/opengl.hpp"

class TextureImpl
{
public:
  GLuint handle;
  Size   size;

  TextureImpl(const SoftwareSurfacePtr& src, const Rect& srcrect) :
    handle(),
    size(srcrect.get_size())
  {
    assert(src);

    assert_gl("TextureImpl enter");

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

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

    glPixelStorei(GL_UNPACK_ALIGNMENT,  1);

#ifdef HAVE_OPENGLES2
    SoftwareSurfacePtr subsurf = src->crop(srcrect);

    glTexImage2D(GL_TEXTURE_2D, 0, gl_format,
                 size.width, size.height,
                 0, /* border */
                 gl_format,
                 GL_UNSIGNED_BYTE,
                 subsurf->get_data());
#else
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src->get_width());

    glTexImage2D(GL_TEXTURE_2D, 0, gl_format,
                 size.width, size.height,
                 0, /* border */
                 gl_format,
                 GL_UNSIGNED_BYTE,
                 src->get_data() + (src->get_pitch() * srcrect.top) + (srcrect.left * src->get_bytes_per_pixel()));
#endif

    assert_gl("packing image texture");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    assert_gl("setting texture parameters");
  }

  ~TextureImpl()
  {
    glDeleteTextures(1, &handle);
  }
};

TexturePtr
Texture::create(const SoftwareSurfacePtr& src, const Rect& srcrect)
{
  return TexturePtr(new Texture(src, srcrect));
}

Texture::Texture(const SoftwareSurfacePtr& src, const Rect& srcrect) :
  impl(new TextureImpl(src, srcrect))
{
}

void
Texture::bind()
{
  glBindTexture(GL_TEXTURE_2D, impl->handle);
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
