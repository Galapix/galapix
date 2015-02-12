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

#include <iostream>

#include "display/framebuffer.hpp"
#include "math/rect.hpp"

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
      texture->bind();
      glEnable(GL_BLEND);
      glEnable(GL_TEXTURE_2D);
      glColor3f(1.0f, 1.0f, 1.0f);

      const float left = srcrect.left / texture->get_width();
      const float top = srcrect.top / texture->get_height();
      const float right = srcrect.right / texture->get_width();
      const float bottom = srcrect.bottom / texture->get_height();

      glBegin(GL_QUADS);
      glTexCoord2f(left, top);
      glVertex2f(dstrect.left, dstrect.top);

      glTexCoord2f(right, top);
      glVertex2f(dstrect.right, dstrect.top);

      glTexCoord2f(right, bottom);
      glVertex2f(dstrect.right, dstrect.bottom);

      glTexCoord2f(left, bottom);
      glVertex2f(dstrect.left, dstrect.bottom);
      glEnd();
    }
  }

  void draw(const Rectf& rect)
  {
    if (texture)
    {
      texture->bind();
      glEnable(GL_BLEND);
      glEnable(GL_TEXTURE_2D);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor3f(1.0f, 1.0f, 1.0f);

      glBegin(GL_QUADS);
      glTexCoord2f(uv.left, uv.top);
      glVertex2f(rect.left, rect.top);

      glTexCoord2f(uv.right, uv.top);
      glVertex2f(rect.right, rect.top);

      glTexCoord2f(uv.right, uv.bottom);
      glVertex2f(rect.right, rect.bottom);

      glTexCoord2f(uv.left, uv.bottom);
      glVertex2f(rect.left, rect.bottom);
      glEnd();
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
