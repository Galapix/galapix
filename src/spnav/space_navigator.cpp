/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "spnav/space_navigator.hpp"

#include <SDL.h>
#include <iostream>
#include <spnav.h>

SpaceNavigator::SpaceNavigator()
{
}

SpaceNavigator::~SpaceNavigator()
{
}

void
SpaceNavigator::run()
{
  if (spnav_open() != 0)
    {
      std::cout << "Error: SpaceNavigator: open failed" << std::endl;
    }
  else
    {
      spnav_event* spnav_ev;
      while(spnav_wait_event(spnav_ev = new spnav_event))
        {
          SDL_Event event;
          event.type = SDL_USEREVENT;
          event.user.code  = 0;
          event.user.data1 = spnav_ev;
          event.user.data2 = 0;

          while (SDL_PushEvent(&event) != 0) {}
        }

      if (spnav_close() != 0)
        {
          std::cout << "Error: SpaceNavigator: close" << std::endl;
        }
    }
}

/* EOF */
