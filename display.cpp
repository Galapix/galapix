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
#include "display.hpp"

SDL_Surface* Display::screen = 0;
Uint32 Display::flags = 0;

void
Display::init()
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
      std::cout << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
      exit(1);
    }
  atexit(SDL_Quit); 
 
  flags = SDL_RESIZABLE;
  screen = SDL_SetVideoMode(1024, 768, 0, flags);
    
  if (screen == NULL) 
    {
      std::cout << "Unable to set video mode: " << SDL_GetError() << std::endl;
      exit(1);
    }

  SDL_WM_SetCaption("Griv 0.0.1", 0 /* icon */);

  SDL_EnableUNICODE(1);
}

void
Display::toggle_fullscreen()
{
  if (flags & SDL_FULLSCREEN)
    flags = SDL_RESIZABLE;
  else
    flags |= SDL_FULLSCREEN;
 
  // Should use desktop resolution for this instead, but how?
  screen = SDL_SetVideoMode(1152, 864, 0, flags); 
}

void
Display::resize(int w, int h)
{
  screen = SDL_SetVideoMode(w, h, 0, SDL_RESIZABLE);
}

void
Display::flip()
{
  SDL_Flip(screen);  
}

void
Display::clear()
{
  SDL_FillRect(Display::get_screen(), NULL, SDL_MapRGB(Display::get_screen()->format, 0, 0, 0));
}

/* EOF */
