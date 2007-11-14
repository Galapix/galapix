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

SDL_Surface* Image::loading_16;
SDL_Surface* Image::loading_32;
SDL_Surface* Image::loading_64;
SDL_Surface* Image::loading_128;
SDL_Surface* Image::loading_256;
SDL_Surface* Image::loading_512;
SDL_Surface* Image::loading_1024;

void 
Image::init()
{
  std::cout << "Image" << std::endl;

  loading_1024 = IMG_Load("loading_1024.jpg");
  loading_512  = IMG_Load("loading_512.jpg");
  loading_256  = IMG_Load("loading_256.jpg");
  loading_128  = IMG_Load("loading_128.jpg");
  loading_64   = IMG_Load("loading_64.jpg");
  loading_32   = IMG_Load("loading_32.jpg");
  loading_16   = IMG_Load("loading_16.jpg");
}

Image::Image(const std::string& url)
  : url(url), 
    surface(0),
    texture(0),
    texture_16x16(0),
    res(0),
    want_res(0),
    image_requested(false)
{
  mutex = SDL_CreateMutex();
}

Image::~Image()
{
  SDL_DestroyMutex(mutex);
}

void
Image::receive(SDL_Surface* new_surface)
{ 
  SDL_LockMutex(mutex);
  if (new_surface)
    {
      if (surface)
        {
          SDL_Surface* old_surface = surface;
          surface = new_surface;
          SDL_FreeSurface(old_surface); 
        }
      else
        {
          surface = new_surface;
        }
      
      res = std::max(surface->w, surface->h);

      force_redraw = true;
    }
  image_requested = false;
  SDL_UnlockMutex(mutex);
}

void
Image::draw(int x, int y, int res)
{
  SDL_LockMutex(mutex);
  if (x > Framebuffer::get_width() ||
      y > Framebuffer::get_height() ||
      x < -res || 
      y < -res)
    { // Image out of screen
      if (res >= 512)
        if (surface)
          {
            SDL_FreeSurface(surface);
            surface = 0;

            delete texture;
            texture = 0;

            want_res  = 0;
            this->res = 0;
          }
    }
  else
    { // image on screen
      //std::cout << round_res(res) << " " << this->res << std::endl;

      if (!image_requested)
        if (surface == 0 || round_res(res) != this->res)
          {
            loader.request(this);
            want_res = round_res(res);
            image_requested = true;
          }

      SDL_Rect dstrect;
      dstrect.x = x;
      dstrect.y = y;
      
      if (0)
        {
          if (surface)
            SDL_BlitSurface(surface, NULL, Framebuffer::get_screen(), &dstrect);
          else if (image_requested)
            SDL_BlitSurface(placeholder(res), NULL, Framebuffer::get_screen(), &dstrect);        
        }
      else
        {
          if (surface)
            {
              if (!texture || texture->surface != surface)
                {
                  if (!texture_16x16)
                    {
                      texture_16x16 = new Texture(surface);
                    }
                  else
                    {
                      delete texture;
                      texture = new Texture(surface);
                    }
                }
            }

          if (texture || texture_16x16)
            {
              if (texture)
                texture->bind();
              else if (texture_16x16)
                texture_16x16->bind();

              glColor3f(1.0f, 1.0f, 1.0f);
              glBegin(GL_QUADS);
              glTexCoord2f(0,0);
              glVertex2i(x, y);

              glTexCoord2f(1,0);
              glVertex2i(x + res, y);

              glTexCoord2f(1,1);
              glVertex2i(x + res, y + res);

              glTexCoord2f(0,1);
              glVertex2i(x, y + res);
              glEnd();
            }
        }
    }
  SDL_UnlockMutex(mutex);
}

int
Image::round_res(int r)
{
  if (r <= 32)
    return 16;
  else if (r <= 64)
    return 32;
  else if (r <= 128)
    return 64;
  else if (r <= 256)
    return 128;
  else if (r <= 512)
    return 256;
  else if (r < 1024)
    return 512;
  else // if (r <= 1024) // zoom limit, 2048 textures make the thing crash
    return 1024;
}

/* EOF */
