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

#include "display/surface.hpp"

#include <glm/ext.hpp>

#include "display/framebuffer.hpp"
#include "display/shader.hpp"
#include "math/rect.hpp"
#include "util/opengl.hpp"

using namespace surf;

class SurfaceImpl
{
public:
  Texture texture;
  Rectf   uv;
  Size    size;

  SurfaceImpl(SoftwareSurface const& src, Rect const& srcrect) :
    texture(),
    uv(),
    size()
  {
    texture = Texture::create(src, srcrect);

    uv = Rectf(Vector2f(0, 0), Sizef(1.0f, 1.0f));

    size = srcrect.size();
  }

  ~SurfaceImpl()
  {
  }

  void draw(const Rectf& srcrect, const Rectf& dstrect) const
  {
    assert_gl("SurfaceImpl::draw enter");

    if (texture)
    {
      const float left = srcrect.left() / static_cast<float>(texture.get_width());
      const float top = srcrect.top() / static_cast<float>(texture.get_height());
      const float right = srcrect.right() / static_cast<float>(texture.get_width());
      const float bottom = srcrect.bottom() / static_cast<float>(texture.get_height());

      std::array<float, 2*4> texcoords = {{
          left, top,
          right, top,
          right, bottom,
          left, bottom,
        }};

      std::array<float, 2*4> positions = {{
          dstrect.left(), dstrect.top(),
          dstrect.right(), dstrect.top(),
          dstrect.right(), dstrect.bottom(),
          dstrect.left(), dstrect.bottom(),
        }};

      GLuint texcoords_vbo;
      glGenBuffers(1, &texcoords_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * texcoords.size(), texcoords.data(), GL_STATIC_DRAW);

      GLuint positions_vbo;
      glGenBuffers(1, &positions_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

      {
        texture.bind();

        glUseProgram(Framebuffer::s_textured_prg);
        glUniform1i(get_uniform_location(Framebuffer::s_textured_prg, "tex"), 0);

        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_textured_prg, "projection"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_textured_prg, "modelview"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

        GLint position_loc = get_attrib_location(Framebuffer::s_textured_prg, "position");
        GLint texcoord_loc = get_attrib_location(Framebuffer::s_textured_prg, "texcoord");

        glEnableVertexAttribArray(texcoord_loc);
        glEnableVertexAttribArray(position_loc);

        glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
        glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
        glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(position_loc);
        glDisableVertexAttribArray(texcoord_loc);

        glUseProgram(0);
      }

      glDeleteBuffers(1, &texcoords_vbo);
      glDeleteBuffers(1, &positions_vbo);
    }

    assert_gl("SurfaceImpl::draw leave");
  }

  void draw(const Rectf& rect)  const
  {
    assert_gl("SurfaceImpl::draw enter");

    if (texture)
    {
      const std::array<float, 2*4> texcoords = {{
          uv.left(), uv.top(),
          uv.right(), uv.top(),
          uv.right(), uv.bottom(),
          uv.left(), uv.bottom(),
        }};

      const std::array<float, 2*4> positions = {{
          rect.left(), rect.top(),
          rect.right(), rect.top(),
          rect.right(), rect.bottom(),
          rect.left(), rect.bottom(),
        }};

      GLuint texcoords_vbo;
      glGenBuffers(1, &texcoords_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * texcoords.size(), texcoords.data(), GL_STATIC_DRAW);

      GLuint positions_vbo;
      glGenBuffers(1, &positions_vbo);
      glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * positions.size(), positions.data(), GL_STATIC_DRAW);

      {
        texture.bind();

        glUseProgram(Framebuffer::s_textured_prg);
        glUniform1i(get_uniform_location(Framebuffer::s_textured_prg, "tex"), 0);

        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_textured_prg, "projection"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_projection));
        glUniformMatrix4fv(get_uniform_location(Framebuffer::s_textured_prg, "modelview"),
                           1, GL_FALSE, glm::value_ptr(Framebuffer::s_modelview));

        GLint position_loc = get_attrib_location(Framebuffer::s_textured_prg, "position");
        GLint texcoord_loc = get_attrib_location(Framebuffer::s_textured_prg, "texcoord");

        glEnableVertexAttribArray(texcoord_loc);
        glEnableVertexAttribArray(position_loc);

        glBindBuffer(GL_ARRAY_BUFFER, texcoords_vbo);
        glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, positions_vbo);
        glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glDisableVertexAttribArray(position_loc);
        glDisableVertexAttribArray(texcoord_loc);

        glUseProgram(0);
      }

      glDeleteBuffers(1, &texcoords_vbo);
      glDeleteBuffers(1, &positions_vbo);
    }

    assert_gl("SurfaceImpl::draw leave");
  }

  void draw(const Vector2f& pos) const
  {
    draw(Rectf(pos, Sizef(size)));
  }
};

Surface
Surface::create(SoftwareSurface const& src, Rect const& srcrect)
{
  return Surface(src, srcrect);
}

Surface
Surface::create(SoftwareSurface const& src)
{
  return Surface(src);
}

Surface::Surface() :
  m_impl()
{
}

Surface::Surface(SoftwareSurface const& src) :
  m_impl(std::make_shared<SurfaceImpl>(src, Rect(Vector2i(0, 0), src.get_size())))
{
}

Surface::Surface(SoftwareSurface const& src, Rect const& srcrect)
  : m_impl(std::make_shared<SurfaceImpl>(src, srcrect))
{
}

void
Surface::draw(const Vector2f& pos) const
{
  if (m_impl)
  {
    m_impl->draw(pos);
  }
}

void
Surface::draw(const Rectf& srcrect, const Rectf& dstrect) const
{
  if (m_impl)
  {
    m_impl->draw(srcrect, dstrect);
  }
}

void
Surface::draw(const Rectf& rect) const
{
  if (m_impl)
  {
    m_impl->draw(rect);
  }
}

int
Surface::get_width() const
{
  if (m_impl)
  {
    return m_impl->size.width();
  }
  else
  {
    return 0;
  }
}

int
Surface::get_height() const
{
  if (m_impl)
  {
    return m_impl->size.height();
  }
  else
  {
    return 0;
  }
}

Size
Surface::get_size() const
{
  if (m_impl)
  {
    return m_impl->size;
  }
  else
  {
    return {};
  }
}

/* EOF */
