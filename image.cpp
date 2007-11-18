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
#include <GL/gl.h>
#include "SDL_image.h"
#include "texture.hpp"
#include "image.hpp"
#include "surface.hpp"

Image::Image(const std::string& url)
  : url(url), 
    requested_res(0),

    received_surface(0),
    received_surface_res(0),

    surface(0),
    surface_16x16(0),

    x_pos(0),
    y_pos(0),
    last_x_pos(0),
    last_y_pos(0)
{
  mutex = SDL_CreateMutex();
}

Image::~Image()
{
  SDL_DestroyMutex(mutex);
}

void
Image::receive(SDL_Surface* new_surface, int r)
{ 
  SDL_LockMutex(mutex);
  
  if (new_surface)
    {
      received_surface     = new_surface;
      received_surface_res = r;
      force_redraw         = true;
    }
  requested_res        = 0; 
 
  SDL_UnlockMutex(mutex);
}

void
Image::draw(float x_offset, float y_offset, float res)
{
  float x = x_pos * res + x_offset;
  float y = y_pos * res + y_offset;

  SDL_LockMutex(mutex);
  if (x > Framebuffer::get_width() ||
      y > Framebuffer::get_height() ||
      x < -res || 
      y < -res)
    { // Image out of screen
      if (surface && surface->get_resolution() >= 512) // keep small images around a while longer
        {
          delete surface;
          surface = 0;
        }
    }
  else
    { // image on screen

      // Handle loading when resolution changed
      if (surface == 0 || 
          round_res(int(res)) != surface->get_resolution())
        {
          if (round_res(int(res)) != requested_res)
            {    
              loader.request(this);
              requested_res = round_res(int(res));
            }
        }
      
      // Handle OpenGL Texture creation when new surface was received
      if (received_surface)
        {
          if (!surface_16x16)
            { // Use surface as the smallest possible surface
              // FIXME: When somebody is fast this could mean a non 16x16 surface
              surface_16x16 = new Surface(received_surface, received_surface_res);
            }
          else
            { // Replace the current surface
              delete surface;
              surface = new Surface(received_surface, received_surface_res);
            }

          received_surface     = 0;
          received_surface_res = 0;
        }

      // Handle drawing
      if (surface)
        surface->draw(x, y, res, res);
      else if (surface_16x16)
        surface_16x16->draw(x, y, res, res);
    }
  SDL_UnlockMutex(mutex);
}

int
Image::round_res(int r)
{
  if (!highquality)
    { // low quality
      if (r < 32)
        return 16;
      else if (r < 64)
        return 32;
      else if (r < 128)
        return 64;
      else if (r < 256)
        return 128;
      else if (r < 512)
        return 256;
      else if (r < 1024)
        return 512;
      else // if (r < 1024) // zoom limit, 2048 textures make the thing crash
        return 1024;
    }
  else
    { // high quality
      if (r <= 16)
        return 16;
      else if (r <= 32)
        return 32;
      else if (r <= 64)
        return 64;
      else if (r <= 128)
        return 128;
      else if (r <= 256)
        return 256;
      else if (r < 512)
        return 512;
      else // if (r <= 1024) // zoom limit, 2048 textures make the thing crash
        return 1024;      
    }
}

void
Image::set_pos(float x, float y)
{
  target_x_pos = x;
  target_y_pos = y;
  last_x_pos = x_pos;
  last_y_pos = y_pos;
}

void
Image::update(float alpha)
{
  if (alpha == 1.0f)
    {
      x_pos = target_x_pos; 
      y_pos = target_y_pos;
      last_x_pos = target_x_pos; 
      last_y_pos = target_y_pos; 
    }
  else
    {
      x_pos = (alpha) * target_x_pos + (1.0f - alpha) * last_x_pos;
      y_pos = (alpha) * target_y_pos + (1.0f - alpha) * last_y_pos;
    }
}

/* EOF */
