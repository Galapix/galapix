/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <iostream>
#include <assert.h>
#include "framebuffer.hpp"
#include "math.hpp"
#include "math/vector2i.hpp"
#include "math/vector2f.hpp"
#include "math/size.hpp"
#include "math/rect.hpp"
#include "software_surface.hpp"
#include "surface.hpp"

class SurfaceImpl
{
public:
  Texture texture;
  Rectf   uv;

  Size size;
  
  SurfaceImpl(const SoftwareSurface& src, const Rect& srcrect)
  {
    assert(src);

    Size texture_size(Math::round_to_power_of_two(srcrect.get_width()),
                      Math::round_to_power_of_two(srcrect.get_height()));

    texture = Texture(texture_size, src, srcrect);
    
    uv = Rectf(Vector2f(0, 0),
               Sizef(float(srcrect.get_width())  / texture_size.width,
                     float(srcrect.get_height()) / texture_size.height));

    size = Size(srcrect.get_size());
  }
  
  ~SurfaceImpl()
  {
  }

  void draw(const Rectf& rect)
  {
    if (texture)
      {
        texture.bind();
        glEnable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        
        // Since FreeImage is giving us the images upside down, we
        // place them at the bottom of the texture instead of the top
        // to avoid blending artifacts
        glBegin(GL_QUADS);
        glTexCoord2f(      0, 1.0f);
        glVertex2f(rect.left, rect.top);

        glTexCoord2f(uv.get_width(), 1.0f);
        glVertex2f(rect.right, rect.top);

        glTexCoord2f(uv.get_width(), 1.0f - uv.get_height());
        glVertex2f(rect.right, rect.bottom);

        glTexCoord2f(      0,  1.0f - uv.get_height());
        glVertex2f(rect.left, rect.bottom);
        glEnd();
      }   
  }

  void draw(const Vector2f& pos)
  {
    draw(Rectf(pos, size));
  }
};

Surface::Surface()
{
}

Surface::Surface(boost::shared_ptr<SurfaceImpl> impl_)
  : impl(impl_)
{
}

Surface::Surface(const SoftwareSurface& src)
  : impl(new SurfaceImpl(src, Rect(Vector2i(0, 0), src.get_size())))
{
}

Surface::Surface(const SoftwareSurface& src, const Rect& srcrect)
  : impl(new SurfaceImpl(src, srcrect))
{
}

Surface::~Surface()
{
}

void
Surface::draw(const Vector2f& pos)
{
  if (impl.get())
    impl->draw(pos);
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

Surface
Surface::get_section(const Rect& rect) const
{
  boost::shared_ptr<SurfaceImpl> surface_impl;

  surface_impl->texture = impl->texture;
  surface_impl->uv      = impl->uv; assert(!"FIXME: Insert code here");
  surface_impl->size    = rect.get_size();

  return Surface(impl);
}

/* EOF */
