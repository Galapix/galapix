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

#include <boost/bind.hpp>
#include "file_entry.hpp"
#include "workspace.hpp"
#include "framebuffer.hpp"
#include "viewer.hpp"
#include "viewer_thread.hpp"
#include "tile_generator_thread.hpp"
#include "space_navigator.hpp"
#include "database_thread.hpp"

ViewerThread* ViewerThread::current_ = 0;

ViewerThread::ViewerThread()
{
  current_ = this;
}

ViewerThread::~ViewerThread()
{
}

void
ViewerThread::receive_file(const FileEntry& entry)
{
  file_queue.push(entry);
}

int
ViewerThread::run()
{
  Workspace workspace;

  Framebuffer::set_video_mode(Size(800, 600));

  workspace.layout(4,3);

  Viewer viewer;

  Uint32 ticks = SDL_GetTicks();

  SpaceNavigator space_navigator;

  while(!viewer.done())
    {     
      while (!file_queue.empty())
        {
          const FileEntry& entry = file_queue.front();
          workspace.add_image(entry);
          file_queue.pop();
        }

      space_navigator.poll(viewer);

      SDL_Event event;
      while (SDL_PollEvent(&event))
        {
          viewer.process_event(workspace, event);
        }

      Uint32 cticks = SDL_GetTicks();
      float delta = (cticks - ticks) / 1000.0f;
      ticks = cticks;

      viewer.update(workspace, delta);

      if (1) // if something has changed, redraw
        {
          Framebuffer::clear();
          viewer.draw(workspace);
          Framebuffer::flip();
        }

      SDL_Delay(30);
    }

  std::cout << "ViewerThread: done" << std::endl;

  return 0;
}

/* EOF */
