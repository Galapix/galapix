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

#include <assert.h>
#include "display.hpp"
#include "surface.hpp"

Surface::Surface(SDL_Surface* surface, int res)
  : surface(surface),
    res(res)
{
  assert(surface);
  texture = new Texture(surface);
}

Surface::~Surface()
{
  SDL_FreeSurface(surface);
}

void
Surface::draw(float x, float y, float w, float h)
{
  if (surface)
    {
      texture->bind();
      
      glColor3f(1.0f, 1.0f, 1.0f);

      glBegin(GL_QUADS);
      glTexCoord2f(0,0);
      glVertex2f(x, y);

      glTexCoord2f(1,0);
      glVertex2f(x + w, y);

      glTexCoord2f(1,1);
      glVertex2f(x + w, y + h);

      glTexCoord2f(0,1);
      glVertex2f(x, y + h);
      glEnd();
    }
}

/* EOF */
