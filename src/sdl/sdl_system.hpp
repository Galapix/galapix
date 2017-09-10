/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_SDL_SDL_SYSTEM_HPP
#define HEADER_GALAPIX_SDL_SDL_SYSTEM_HPP

#include "galapix/system.hpp"

class SDLSystem : public System
{
private:
public:
  SDLSystem();
  ~SDLSystem();

  void launch_viewer(Workspace& workspace, Options& options) override;
  bool requires_command_line_args() override;
  void trigger_redraw() override;
  void set_trackball_mode(bool active) override;

private:
  SDLSystem(const SDLSystem&) = delete;
  SDLSystem& operator=(const SDLSystem&) = delete;
};

#endif

/* EOF */