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

#ifndef HEADER_IMAGE_HPP
#define HEADER_IMAGE_HPP

#include <sstream>
#include "loader.hpp"
#include "griv.hpp"

class Image
{
public:
  static SDL_Surface* loading_16;
  static SDL_Surface* loading_32;
  static SDL_Surface* loading_64;
  static SDL_Surface* loading_128;
  static SDL_Surface* loading_256;
  static SDL_Surface* loading_512;
  static SDL_Surface* loading_1024;

  static void init();

  static SDL_Surface* placeholder(int res)
  {
    if (res == 16)
      return loading_16;
    else if (res == 32)
      return loading_32;
    else if (res == 64)
      return loading_64;
    else if (res == 128)
      return loading_128;
    else if (res == 256)
      return loading_256;
    else if (res == 512)
      return loading_512;
    else if (res == 1024)
      return loading_1024;
    else 
      return loading_1024;
  }

  std::string uid;
  SDL_Surface* surface;
  int res;
  bool image_requested;

  SDL_mutex* mutex;


  Image(const std::string& uid) 
    : uid(uid),
      surface(0),
      res(0),
      image_requested(false)
  {
    mutex = SDL_CreateMutex();
  }

  ~Image()
  {
    SDL_DestroyMutex(mutex);
  }

  void receive(SDL_Surface* new_surface)
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
        force_redraw = true;
      }
    image_requested = false;
    SDL_UnlockMutex(mutex);
  }

  void draw(int x, int y, int res)
  {
    SDL_LockMutex(mutex);
    if (x > screen->w ||
        y > screen->h ||
        x < -res || 
        y < -res)
      { // Image out of screen
        if (res >= 512)
          if (surface)
            {
              SDL_FreeSurface(surface);
              surface = 0;
            }
      }
    else
      { // image on screen
        if (!image_requested)
          if (surface == 0 || res != this->res)
            {
              //std::cout << "Requesting" << std::endl;
              loader.request(uid, res, this);
              image_requested = true;
              this->res = res;
            }

        SDL_Rect dstrect;
        dstrect.x = x;
        dstrect.y = y;
        if (surface)
          SDL_BlitSurface(surface, NULL, screen, &dstrect);
        else if (image_requested)
          SDL_BlitSurface(placeholder(res), NULL, screen, &dstrect);        
      }
    SDL_UnlockMutex(mutex);
  }
};

#endif

/* EOF */
