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

  std::string uid;
  SDL_Surface* surface;
  int res;
  
  Image(const std::string& uid) 
    : uid(uid),
      surface(0),
      res(0)
  {
  }

  void receive(SDL_Surface* new_surface)
  {
    if (surface)
      SDL_FreeSurface(surface);    
    
    surface = new_surface;
  }

  void draw(int x, int y, int res)
  {
    if (x > screen->w ||
        y > screen->h ||
        x < -res || 
        y < -res)
      {
        if (0)
          if (surface)
            {
              SDL_FreeSurface(surface);
              surface = 0;
            }
      }
    else
      {
        if (surface == 0 || res != this->res)
          {
            std::ostringstream out;
            out << config_home << "/.griv/" << res << "/" << uid << ".jpg";
            loader.request(out.str(), this);
            //surface = IMG_Load(out.str().c_str());
            //std::cout << "Loading: " << out.str() << std::endl;
            this->res = res;
          }

        if (surface != 0)
          {
            SDL_Rect dstrect;

            dstrect.x = x;
            dstrect.y = y;

            SDL_BlitSurface(surface, NULL, screen, &dstrect);
          }
      }
  }
};

#endif

/* EOF */
