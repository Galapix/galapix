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
#include "SDL_image.h"
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

/* EOF */
