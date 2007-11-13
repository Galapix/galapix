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
#include <stdexcept>
#include "display.hpp"
#include "texture.hpp"

Texture::Texture(SDL_Surface* surface)
  : handle(0)
{
  glGenTextures(1, &handle);

  width  = surface->w;
  height = surface->h;

  const SDL_PixelFormat* format = surface->format;

  if(format->BitsPerPixel != 24 && format->BitsPerPixel != 32)
    throw std::runtime_error("image has not 24 or 32 bit color depth");
  
  GLint maxt;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxt);
  if(surface->w > maxt || surface->h > maxt)
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

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, surface->pitch/format->BytesPerPixel);

  {
    int res = std::max(surface->w, surface->h);
    std::cout << "Res: " << res << std::endl;
    unsigned char dummy[res*res*3];
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 res,
                 res,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 dummy);
  }
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 
                  0, 0, surface->w, surface->h, sdl_format,
                  GL_UNSIGNED_BYTE, surface->pixels);

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

/* EOF */
