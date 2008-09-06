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

#ifndef HEADER_VIEWER_HPP
#define HEADER_VIEWER_HPP

#include <memory>
#include "SDL.h"
#include "surface.hpp"
#include "math/vector2i.hpp"
#include "math/vector2f.hpp"
#include "viewer_state.hpp"
#include "pan_tool.hpp"

class Tool;
class Workspace;

class Viewer
{
private:
  bool  quit;
  bool  force_redraw;
  bool  draw_grid;
  float gamma;

  ViewerState state;

  std::auto_ptr<PanTool> pan_tool;
  PanTool* current_tool;

public:
  Viewer();

  void draw(Workspace& workspace);
  void update(Workspace& workspace, float delta);
  void process_event(Workspace& workspace, const SDL_Event& event);
  bool done() const { return quit; }

  ViewerState& get_state() { return state; }

private:
  Viewer (const Viewer&);
  Viewer& operator= (const Viewer&);
};

#endif

/* EOF */
