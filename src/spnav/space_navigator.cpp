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
#include <assert.h>
#include <iostream>
#include <spnav.h>
#include <unistd.h>

#include "util/log.hpp"
#include "util/select.hpp"

SpaceNavigator::SpaceNavigator() :
  m_quit{true},
  m_thread{},
  m_pipefd{-1,-1}
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
    if (pipe(m_pipefd) < 0)
    {
      std::cout << "Error: SpaceNavigator: pipe() failed" << std::endl;
    }

    Select sel;

    while(!m_quit)
    {
      sel.clear();
      sel.add_fd(m_pipefd[0]);
      sel.add_fd(spnav_fd());

      if (sel.wait() > 0)
      {
        if (sel.is_ready(spnav_fd()))
        {
          spnav_event spnav_ev;
          while(spnav_poll_event(&spnav_ev))
          {
            SDL_Event event;

            event.type = SDL_USEREVENT;
            event.user.code  = 0;
            event.user.data1 = new spnav_event(spnav_ev);
            event.user.data2 = 0;

            while (SDL_PushEvent(&event) != 1) {}
          }
        }
      }
    }

#if 0
    // old obsolete code
    spnav_event* spnav_ev;
    while(spnav_wait_event(spnav_ev = new spnav_event))
    {
      SDL_Event event;
      event.type = SDL_USEREVENT;
      event.user.code  = 0;
      event.user.data1 = spnav_ev;
      event.user.data2 = 0;

      while (SDL_PushEvent(&event) != 1) {}
    }
#endif

    if (spnav_close() != 0)
    {
      std::cout << "Error: SpaceNavigator: close" << std::endl;
    }

    close(m_pipefd[0]);
    close(m_pipefd[1]);
  }
}

void
SpaceNavigator::start_thread()
{
  m_quit = false;
  m_thread = std::thread(std::bind(&SpaceNavigator::run, this));
}

void
SpaceNavigator::stop_thread()
{
  assert(!m_quit);

  m_quit = true;

  // write some data to the pipe to wake up the select() call
  char data[]{0};
  write(m_pipefd[1], data, 1);

  m_thread.join();
}

/* EOF */
