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
#include "software_surface.hpp"
#include "surface.hpp"

Surface::Surface(const SoftwareSurface& surface, int x, int y, int w, int h)
  : texture(0)
{
  assert(surface);

  tex_w = Math::round_to_power_of_two(w);
  tex_h = Math::round_to_power_of_two(h);

  if (tex_w <= 1024 && tex_h <= 1024)
    {
      texture = new Texture(tex_w, tex_h, 
                            surface,
                            x, y, w, h);
    
      u = float(w) / tex_w;
      v = float(h) / tex_h;

      aspect = float(w) / h;

      width  = w;
      height = h;
    }
  else
    {
      std::cout << "Image violates maximum texture size: "
                << surface.get_width() << "x" << surface.get_height() << std::endl;
    }
}

Surface::~Surface()
{
  delete texture;
}

void
Surface::draw(float x, float y)
{
  draw(x, y, width, height);
}

void
Surface::draw(float x, float y, float orig_w, float orig_h)
{
  if (texture)
    {
      texture->bind();
      
      glColor3f(1.0f, 1.0f, 1.0f);

      if (0)
        {
          float w, h;
          if (aspect > 1.0f)
            { // FIXME: This only works as long as w == h
              w = orig_w;
              h = orig_h / aspect;
            }
          else
            {
              w = orig_w * aspect;
              h = orig_h;
            }

          x += (orig_w - w)/2;
          y += (orig_h - h)/2;
        }
      float w = orig_w;
      float h = orig_h;

      glBegin(GL_QUADS);
      glTexCoord2f(0,0);
      glVertex2f(x, y);

      glTexCoord2f(u,0);
      glVertex2f(x + w, y);

      glTexCoord2f(u,v);
      glVertex2f(x + w, y + h);

      glTexCoord2f(0,v);
      glVertex2f(x, y + h);
      glEnd();
    }
}

/* EOF */
