// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "app/app_system.hpp"

#include <SDL.h>

#include "galapix/options.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "app/app_viewer.hpp"

namespace galapix {

AppSystem::AppSystem()
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
  {
    std::ostringstream os;
    os << "unable to initialize SDL: " << SDL_GetError();
    throw std::runtime_error(os.str());
  }
  else
  {
    atexit(SDL_Quit);
  }
}

AppSystem::~AppSystem()
{
}

void
AppSystem::launch_viewer(Workspace& workspace, Options& opts)
{
  Viewer viewer(*this, &workspace);
  AppViewer app_viewer(opts.geometry, opts.fullscreen, opts.anti_aliasing, viewer);
  viewer.layout_tight();
  viewer.zoom_to_selection();
  app_viewer.run();
}

bool
AppSystem::requires_command_line_args()
{
  // Empty argv opens an empty workspace; do not print --help.
  return false;
}

void
AppSystem::trigger_redraw()
{
  SDL_Event event;
  event.type = SDL_USEREVENT;
  event.user.code  = 1;
  event.user.data1 = nullptr;
  event.user.data2 = nullptr;

  // Do not spin if the queue is full — Viewer::redraw already coalesces.
  SDL_PushEvent(&event);
}

void
AppSystem::set_trackball_mode(bool active)
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

} // namespace galapix

/* EOF */
