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
  Size    texture_size;
  Sizef   uv;

  Size size;
  
  SurfaceImpl(const SoftwareSurface& src, const Rect& srcrect)
  {
    assert(src);

    texture_size = Size(Math::round_to_power_of_two(srcrect.get_width()),
                        Math::round_to_power_of_two(srcrect.get_height()));

    texture = Texture(texture_size, src, srcrect);
    
    uv = Sizef(float(srcrect.get_width())  / texture_size.width,
               float(srcrect.get_height()) / texture_size.height);

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
      
        glColor3f(1.0f, 1.0f, 1.0f);
        
        glBegin(GL_QUADS);
        glTexCoord2f(      0, uv.height);
        glVertex2f(rect.left, rect.top);

        glTexCoord2f(uv.width, uv.height);
        glVertex2f(rect.right, rect.top);

        glTexCoord2f(uv.width, 0);
        glVertex2f(rect.right, rect.bottom);

        glTexCoord2f(      0,  0);
        glVertex2f(rect.left, rect.bottom);
        glEnd();
      }   
  }

  void draw(const Vector2f& pos)
  {
    draw(Rectf(pos, size));
  }
};

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
  impl->draw(pos);
}

void
Surface::draw(const Rectf& rect)
{
  impl->draw(rect);
}

int
Surface::get_width() const 
{
  return impl->size.width; 
}

int
Surface::get_height() const
{
  return impl->size.height; 
}

/* EOF */
