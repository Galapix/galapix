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
#include "display.hpp"
#include "loader.hpp"
#include "griv.hpp"

class Texture;

class Image
{
public:
  // Place holder images when no other is available
  static SDL_Surface* loading_16;
  static SDL_Surface* loading_32;
  static SDL_Surface* loading_64;
  static SDL_Surface* loading_128;
  static SDL_Surface* loading_256;
  static SDL_Surface* loading_512;
  static SDL_Surface* loading_1024;

  static void init();
  static void deinit() {}

  static SDL_Surface* placeholder(int res)
  {
    if (res < 16)
      return loading_16;
    else if (res < 32)
      return loading_32;
    else if (res < 64)
      return loading_64;
    else if (res < 128)
      return loading_128;
    else if (res < 256)
      return loading_256;
    else if (res < 512)
      return loading_512;
    else if (res < 1024)
      return loading_1024;
    else 
      return loading_1024;
  }

public:
  std::string url;
  
  SDL_Surface* surface;
  Texture*     texture;
  Texture*     texture_16x16;
  int res;
  int want_res;
  bool image_requested;

  SDL_mutex* mutex;

  float x_pos;
  float y_pos;

  Image(const std::string& url);
  ~Image();

  void receive(SDL_Surface* new_surface);
  void draw(float x_offset, float y_offset, float res);

  int round_res(int res);
};

#endif

/* EOF */
