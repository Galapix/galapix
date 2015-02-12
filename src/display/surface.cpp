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

#include "display/surface.hpp"

#include <glm/ext.hpp>

#include "display/shader.hpp"
#include "display/framebuffer.hpp"
#include "math/rect.hpp"

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

class SurfaceImpl
{
public:
  TexturePtr texture;
  Rectf   uv;
  Size    size;

  SurfaceImpl(const SoftwareSurfacePtr& src, const Rect& srcrect) :
    texture(),
    uv(),
    size()
  {
    assert(src);

    texture = Texture::create(src, srcrect);

    uv = Rectf(Vector2f(0, 0), Sizef(1.0f, 1.0f));

    size = Size(srcrect.get_size());
  }

  ~SurfaceImpl()
  {
  }

  void draw(const Rectf& srcrect, const Rectf& dstrect)
  {
    if (texture)
    {
      const float left = srcrect.left / texture->get_width();
      const float top = srcrect.top / texture->get_height();
      const float right = srcrect.right / texture->get_width();
      const float bottom = srcrect.bottom / texture->get_height();

      std::array<float, 2*4> texcoords = {
        left, top,
        right, top,
        right, bottom,
        left, bottom,
      };

      std::array<float, 2*4> positions = {
        dstrect.left, dstrect.top,
        dstrect.right, dstrect.top,
        dstrect.right, dstrect.bottom,
        dstrect.left, dstrect.bottom,
      };

      if (true)
      {
        texture->bind();

        glUseProgram(Framebuffer::s_texured_prg);
        glUniform1i(get_uniform_location(Framebuffer::s_texured_prg, "tex"), 0);

        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_texured_prg, "projection"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_texured_prg, "modelview"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

        GLint position_loc = get_attrib_location(Framebuffer::s_texured_prg, "position");
        GLint texcoord_loc = get_attrib_location(Framebuffer::s_texured_prg, "texcoord");

        glEnableVertexAttribArray(texcoord_loc);
        glEnableVertexAttribArray(position_loc);

        glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, texcoords.data());
        glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, positions.data());

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(position_loc);
        glDisableVertexAttribArray(texcoord_loc);

        glUseProgram(0);
      }
      else
      {
        texture->bind();

        glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        glTexCoordPointer(2, GL_FLOAT, 0, texcoords.data());
        glVertexPointer(2, GL_FLOAT, 0, positions.data());

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisable(GL_TEXTURE_2D);
      }
    }
  }

  void draw(const Rectf& rect)
  {
    if (texture)
    {
      const std::array<float, 2*4> texcoords = {
        uv.left, uv.top,
        uv.right, uv.top,
        uv.right, uv.bottom,
        uv.left, uv.bottom,
      };

      const std::array<float, 2*4> positions = {
        rect.left, rect.top,
        rect.right, rect.top,
        rect.right, rect.bottom,
        rect.left, rect.bottom,
      };

      if (true)
      {
        texture->bind();

        glUseProgram(Framebuffer::s_texured_prg);
        glUniform1i(get_uniform_location(Framebuffer::s_texured_prg, "tex"), 0);

        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_texured_prg, "projection"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_texured_prg, "modelview"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

        GLint position_loc = get_attrib_location(Framebuffer::s_texured_prg, "position");
        GLint texcoord_loc = get_attrib_location(Framebuffer::s_texured_prg, "texcoord");

        glEnableVertexAttribArray(texcoord_loc);
        glEnableVertexAttribArray(position_loc);
        
        glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, texcoords.data());
        glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, positions.data());

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(position_loc);
        glDisableVertexAttribArray(texcoord_loc);

        glUseProgram(0);
      }
      else
      {
        texture->bind();
        glEnable(GL_TEXTURE_2D);

        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);

        glTexCoordPointer(2, GL_FLOAT, 0, texcoords.data());
        glVertexPointer(2, GL_FLOAT, 0, positions.data());

        glColor3f(1.0f, 1.0f, 1.0f);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        glDisable(GL_TEXTURE_2D);
      }
    }
  }

  void draw(const Vector2f& pos)
  {
    draw(Rectf(pos, size));
  }
};

SurfacePtr
Surface::create(const SoftwareSurfacePtr& src, const Rect& srcrect)
{
  return SurfacePtr(new Surface(src, srcrect));
}

SurfacePtr
Surface::create(const SoftwareSurfacePtr& src)
{
  return SurfacePtr(new Surface(src));
}

Surface::Surface(const SoftwareSurfacePtr& src) :
  impl(new SurfaceImpl(src, Rect(Vector2i(0, 0), src->get_size())))
{
}

Surface::Surface(const SoftwareSurfacePtr& src, const Rect& srcrect)
  : impl(new SurfaceImpl(src, srcrect))
{
}

void
Surface::draw(const Vector2f& pos)
{
  if (impl.get())
    impl->draw(pos);
}

void
Surface::draw(const Rectf& srcrect, const Rectf& dstrect)
{
  if (impl.get())
    impl->draw(srcrect, dstrect);
}

void
Surface::draw(const Rectf& rect)
{
  if (impl.get())
    impl->draw(rect);
}

int
Surface::get_width() const
{
  if (impl.get())
    return impl->size.width;
  else
    return 0;
}

int
Surface::get_height() const
{
  if (impl.get())
    return impl->size.height;
  else
    return 0;
}

Size
Surface::get_size() const
{
  if (impl.get())
    return impl->size;
  else
    return Size();
}

/* EOF */
