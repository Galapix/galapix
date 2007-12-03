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
#include "filesystem.hpp"
#include "SDL_image.h"
#include "texture.hpp"
#include "software_surface.hpp"
#include "image.hpp"
#include "jpeg.hpp"
#include "cache.hpp"
#include "surface.hpp"
#include "large_surface.hpp"

Image::Image(const std::string& url)
  : url(url), 
    original_width(0),
    original_height(0),
    requested_res(0),

    received_surface(0),
    received_surface_res(0),

    surface_resolution(0),
    surface(0),
    surface_16x16(0),

    x_pos(0),
    y_pos(0),
    last_x_pos(0),
    last_y_pos(0),
    visible(false)
{
  // FIXME: Slow, make this in a seperate thread
  const FileEntry* entry = cache->get_entry(url);
  
  if (entry)
    {
      original_width  = entry->width;
      original_height = entry->height;
      original_mtime  = entry->mtime;
    }

  //JPEG::get_size(url.substr(7), original_width, original_height);
  //original_mtime = Filesystem::get_mtime(url.substr(7));

  //std::cout << url << " " << original_width << "x" << original_height << std::endl;
  
  mutex = SDL_CreateMutex();
}

Image::~Image()
{
  SDL_DestroyMutex(mutex);
}

void
Image::receive(SoftwareSurface* new_surface, int r)
{
  SDL_LockMutex(mutex);
  
  if (new_surface)
    {
      if (received_surface)
        {
          delete received_surface;
          received_surface = 0;
        }

      received_surface     = new_surface;
      received_surface_res = r;
      force_redraw         = true;
    }
  requested_res        = 0; 
 
  SDL_UnlockMutex(mutex);
}

void
Image::draw(float x_offset, float y_offset, float zoom)
{
  float x = x_pos * zoom + x_offset;
  float y = y_pos * zoom + y_offset;

  SDL_LockMutex(mutex);
  if (x > Framebuffer::get_width() ||
      y > Framebuffer::get_height() ||
      x < -zoom || 
      y < -zoom)
    { // Image out of screen
      visible = false;
      if (surface && surface_resolution >= 512) // keep small images around a while longer
        {
          delete surface;
          surface = 0;
        }
    }
  else
    { // image on screen
      visible = true;
      // Handle loading when resolution changed
      if (surface == 0 || zoom2res(zoom) != surface_resolution)
        {
          if (zoom2res(zoom) != requested_res)
            {    
              loader.request(this);
              requested_res = zoom2res(zoom);
            }
        }
      
      // Handle OpenGL Texture creation when new surface was received
      if (received_surface)
        {
          if (!surface_16x16)
            { // Use surface as the smallest possible surface
              // FIXME: When somebody is fast this could mean a non 16x16 surface
              surface_16x16 = new LargeSurface(SWSurfaceHandle(received_surface)); // FIXME: could use Surface instead
            }
          else
            { // Replace the current surface
              delete surface;
              surface = new LargeSurface(SWSurfaceHandle(received_surface));
              surface_resolution = received_surface_res;
            }

          received_surface     = 0;
          received_surface_res = 0;
        }

      // Detect aspect ration
      float aspect = float(original_width)/float(original_height);
      float dw, dh;
      if (aspect > 1.0f)
        {
          dw = zoom;
          dh = zoom / aspect;
        }
      else
        {
          dw = zoom * aspect;
          dh = zoom;
        }

      x += (zoom - dw)/2;
      y += (zoom - dh)/2;

      // Handle drawing
      if (surface)
        {
          surface->draw(x, y, dw, dh);
        }
      else if (surface_16x16)
        {
          surface_16x16->draw(x, y, dw, dh);
        }
    }
  SDL_UnlockMutex(mutex);
}

int
Image::zoom2res(float z)
{
  if (!highquality)
    { // low quality
      if (z < 32)
        return 16;
      else if (z < 64)
        return 32;
      else if (z < 128)
        return 64;
      else if (z < 256)
        return 128;
      else if (z < 512)
        return 256;
      else if (z < 1024)
        return 512;
      else if (z < 1024)
        return 1024;
      else
        {
          if (z > original_width || z > original_height)
            return -1;
          else
            return 1024;
        }
    }
  else
    { // high quality
      if (z <= 16)
        return 16;
      else if (z <= 32)
        return 32;
      else if (z <= 64)
        return 64;
      else if (z <= 128)
        return 128;
      else if (z <= 256)
        return 256;
      else if (z < 512)
        return 512;
      else if (z <= 1024)
        return 1024;      
      else
        return -1;
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
