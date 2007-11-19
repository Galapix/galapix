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
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>
#include <string.h>
#include "display.hpp"
#include "texture.hpp"

Texture::Texture(int width, int height, 
                 SDL_Surface* surface, int s_x, int s_y, int s_w, int s_h)
  : handle(0),
    width(width),
    height(height)
{
  assert(surface);

  if (0)
    std::cout << boost::format(",----------------------------\n"
                               "| Pointer: 0x%p\n"
                               "| Size:    %dx%d\n"
                               "| Pitch:   %d vs %d\n"
                               "| Rmask:   0x%08x\n"
                               "| Gmask:   0x%08x\n"
                               "| Bmask:   0x%08x\n"
                               "| Amask:   0x%08x\n"
                               "| Flags:   0x%08x -> %s%s%s%s\n"
                               "| Palette: 0x%08x\n"
                               "| BitsPerPixel: %d\n"
                               "`----------------------------\n"
                               )
      % surface
      % surface->w
      % surface->h
      % surface->pitch
      % (surface->w*3)
      % surface->format->Rmask
      % surface->format->Gmask
      % surface->format->Bmask
      % surface->format->Amask
      % surface->flags
      % ((surface->flags & SDL_HWSURFACE) ? "HWSURFACE " : "")
      % ((surface->flags & SDL_SWSURFACE) ? "SWSURFACE " : "")
      % ((surface->flags & SDL_SRCCOLORKEY) ? "SRCCOLORKEY " : "")
      % ((surface->flags & SDL_SRCALPHA) ? "SRCALPHA " : "")
      % surface->format->palette
      % static_cast<int>(surface->format->BitsPerPixel);

  glGenTextures(1, &handle);

  const SDL_PixelFormat* format = surface->format;

  if(format->BitsPerPixel != 24 && format->BitsPerPixel != 32)
    throw std::runtime_error("image has not 24 or 32 bit color depth");
  
  GLint maxt;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxt);
  if(width > maxt || height > maxt)
    {
      throw std::runtime_error("Texture size not supported");
    }

  GLint sdl_format;
  if (format->BytesPerPixel == 3)
    {
      sdl_format = GL_RGB;
    }
  else if (format->BytesPerPixel == 4)
    {
      sdl_format = GL_RGBA;
    }
  else
    {
      throw std::runtime_error("Texture: Image format not supported");
    }

  glBindTexture(GL_TEXTURE_2D, handle);
  glEnable(GL_TEXTURE_2D);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,  4);

  { // Create the texture
    unsigned char dummy[width*height*3];
    memset(dummy, 100, width*height*3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 width,
                 height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 dummy);
  }

  assert_gl("packing image texture");

  if (surface->pitch != (surface->w * surface->format->BytesPerPixel))
    std::cout  << surface->pitch << " " << (surface->w * surface->format->BytesPerPixel) << std::endl;

  //std::cout << surface->pitch << " " << s_w << " " << s_h << std::endl;
  glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->w);
  glPixelStorei(GL_UNPACK_ALIGNMENT,  4);

  // Upload the subimage
  glTexSubImage2D(GL_TEXTURE_2D, 0, 
                  0, 0, s_w, s_h, sdl_format,
                  GL_UNSIGNED_BYTE, 
                  static_cast<Uint8*>(surface->pixels) 
                  + (surface->pitch * s_y)
                  + (s_x * surface->format->BytesPerPixel));

  assert_gl("creating texture");

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP);

  assert_gl("setting texture parameters");
}

Texture::~Texture()
{
  glDeleteTextures(1, &handle);
}

void
Texture::bind()
{
  glBindTexture(GL_TEXTURE_2D, handle);
}

int
Texture::get_width() const
{
  return width;
}

int
Texture::get_height() const
{
  return height;
}

/* EOF */
