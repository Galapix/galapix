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

#include "display/texture.hpp"

#include "display/framebuffer.hpp"
#include "math/rect.hpp"
#include "util/opengl.hpp"

using namespace surf;

class TextureImpl
{
public:
  GLuint handle;
  Size   size;

  TextureImpl(SoftwareSurface const& src, Rect const& srcrect) :
    handle(),
    size(srcrect.size())
  {
    assert_gl("TextureImpl enter");

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    GLenum gl_format = GL_RGB;
    switch(src.get_pixel_data().get_format())
    {
      case surf::PixelFormat::RGB:
        gl_format = GL_RGB;
        break;

      case surf::PixelFormat::RGBA:
        gl_format = GL_RGBA;
        break;

      default:
        assert(false && "Texture: Not supposed to be reached");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT,  1);

#ifdef HAVE_OPENGLES2
    SoftwareSurface subsurf = src.crop(srcrect);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(gl_format),
                 size.width, size.height,
                 0, /* border */
                 gl_format,
                 GL_UNSIGNED_BYTE,
                 subsurf.get_pixel_data().get_data());
#else
    PixelData const& pixel_data = src.get_pixel_data();

    glPixelStorei(GL_UNPACK_ROW_LENGTH, src.get_width());

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(gl_format),
                 size.width(), size.height(),
                 0, /* border */
                 gl_format,
                 GL_UNSIGNED_BYTE,
                 pixel_data.get_data() +
                 (pixel_data.get_pitch() * srcrect.top()) +
                 (srcrect.left() * pixel_data.get_bytes_per_pixel()));
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

Texture
Texture::create(SoftwareSurface const& src, Rect const& srcrect)
{
  return Texture(src, srcrect);
}

Texture::Texture() :
  m_impl()
{
}

Texture::Texture(SoftwareSurface const& src, Rect const& srcrect) :
  m_impl(std::make_shared<TextureImpl>(src, srcrect))
{
}

void
Texture::bind() const
{
  if (m_impl)
  {
    glBindTexture(GL_TEXTURE_2D, m_impl->handle);
  }
}

int
Texture::get_width() const
{
  if (!m_impl)
  {
    return 0;
  }
  else
  {
    return m_impl->size.width();
  }
}

int
Texture::get_height() const
{
  if (!m_impl)
  {
    return 0;
  }
  else
  {
    return m_impl->size.height();
  }
}

/* EOF */
