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

#include <math.h>
#include <assert.h>
#include <iostream>
#include <spnav.h>
#include "viewer.hpp"
#include "space_navigator.hpp"

SpaceNavigator::SpaceNavigator()
  : usable(false),
    allow_rotate(false)
{
  if (spnav_open() != 0)
    {
      std::cout << "Error: SpaceNavigator: open failed" << std::endl;
      usable = false;
    }
  else
    {
      usable = true;
    }
}

SpaceNavigator::~SpaceNavigator()
{
  if (usable)
  if (spnav_close() != 0)
    {
      std::cout << "Error: SpaceNavigator: close" << std::endl;
    }
}

void
SpaceNavigator::poll(Viewer& viewer)
{
  if (usable)
    {
      spnav_event event;
      while(spnav_poll_event(&event))
        {
          switch(event.type)
            {
              case SPNAV_EVENT_MOTION:
                {
                  if (0)
                    std::cout << "MotionEvent: " 
                              << "("
                              << event.motion.x << ", "
                              << event.motion.y << ", "
                              << event.motion.z
                              << ") ("
                              << event.motion.rx << ", "
                              << event.motion.ry << ", "
                              << event.motion.rz
                              << std::endl;              


                  float factor = -abs(event.motion.y)/10000.0f;

                  if (event.motion.y > 0)
                    viewer.get_state().zoom(1.0f+factor);
                  else if (event.motion.y < 0)
                    viewer.get_state().zoom(1.0f/(1.0f+factor));

                  viewer.get_state().move(Vector2f(-event.motion.x / 10.0f,
                                                   +event.motion.z / 10.0f));

                  if (allow_rotate)
                    viewer.get_state().rotate(event.motion.ry / 200.0f);
                }
                break;
            
              case SPNAV_EVENT_BUTTON:
                if (0)
                std::cout << "ButtonEvent: " << event.button.press << event.button.bnum << std::endl;

                if (event.button.bnum == 0 && event.button.press)
                  viewer.get_state().set_angle(0.0f);
                
                if (event.button.bnum == 1 && event.button.press)
                  allow_rotate = !allow_rotate;
                break;

              default:
                assert(!"SpaceNavigator: Unhandled event");
            }
        }
    }
}

/* EOF */
