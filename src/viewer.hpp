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

class PanTool;
class Tool;
class Workspace;

class ViewerState
{
private:
  float    scale;
  float    angle;
  Vector2f offset;
  
public:
  ViewerState();

  void zoom(float factor, const Vector2i& pos);
  void zoom(float factor);
  void move(const Vector2f& pos);
  void rotate(float r);
  void set_angle(float r);

  Vector2f screen2world(const Vector2i&) const;
  Rectf    screen2world(const Rect&) const;

  Vector2f get_offset() const { return offset; }
  float    get_scale()  const { return scale; }
  float    get_angle() const  { return angle; }
};

class Viewer
{
private:
  bool quit;
  bool trackball_mode;
  bool force_redraw;
  bool drag_n_drop;
  bool draw_grid;
  int  zoom_button;
  float gamma;

  Vector2i mouse_pos;

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
