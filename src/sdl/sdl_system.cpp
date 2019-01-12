/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
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

#include "sdl/sdl_system.hpp"

#include <SDL.h>

#include "galapix/options.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "sdl/sdl_viewer.hpp"

SDLSystem::SDLSystem()
{
}

SDLSystem::~SDLSystem()
{
}

void
SDLSystem::launch_viewer(Workspace& workspace, Options& opts)
{
  Viewer viewer(*this, &workspace);
  SDLViewer sdl_viewer(opts.geometry, opts.fullscreen, opts.anti_aliasing, viewer);
  viewer.layout_tight();
  viewer.zoom_to_selection();
  sdl_viewer.run();
}

bool
SDLSystem::requires_command_line_args()
{
  return true;
}

void
SDLSystem::trigger_redraw()
{
  SDL_Event event;
  event.type = SDL_USEREVENT;
  event.user.code  = 1;
  event.user.data1 = nullptr;
  event.user.data2 = nullptr;

  while (SDL_PushEvent(&event) != 1) {}
}

void
SDLSystem::set_trackball_mode(bool active)
{
  if (active)
  {
    SDL_SetRelativeMouseMode(SDL_TRUE);
  }
  else
  {
    SDL_SetRelativeMouseMode(SDL_FALSE);
  }
}

/* EOF */
