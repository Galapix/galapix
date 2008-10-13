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

#include <boost/shared_ptr.hpp>
#include <memory>
#include "surface.hpp"
#include "math/vector2i.hpp"
#include "math/vector2f.hpp"
#include "viewer_state.hpp"

class Tool;
class PanTool;
class MoveTool;
class ZoomTool;
class ResizeTool;
class RotateTool;
class ViewRotateTool;
class ZoomRectTool;
class Workspace;

class Viewer
{
private:
  Workspace* workspace;
  bool  draw_grid;
  float gamma;

  ViewerState state;

  boost::shared_ptr<ZoomTool>   keyboard_zoom_in_tool;
  boost::shared_ptr<ZoomTool>   keyboard_zoom_out_tool;
  boost::shared_ptr<ViewRotateTool> keyboard_view_rotate_tool;

  boost::shared_ptr<PanTool>    pan_tool;
  boost::shared_ptr<MoveTool>   move_tool;
  boost::shared_ptr<ZoomRectTool>   zoom_rect_tool;
  boost::shared_ptr<ZoomTool>   zoom_in_tool;
  boost::shared_ptr<ZoomTool>   zoom_out_tool;
  boost::shared_ptr<ResizeTool> resize_tool;
  boost::shared_ptr<RotateTool> rotate_tool;

  Tool* left_tool;
  Tool* middle_tool;
  Tool* right_tool;

  Vector2i mouse_pos;

  int background_color;
  std::vector<RGBA> background_colors;

public:
  Viewer(Workspace* workspace);

  void draw();
  void update(float delta);

  ViewerState& get_state() { return state; }
  Workspace*   get_workspace() { return workspace; }

  void on_key_up(int key);
  void on_key_down(int key);

  void on_mouse_motion(const Vector2i& pos, const Vector2i& rel);
  void on_mouse_button_down(const Vector2i& pos, int btn);
  void on_mouse_button_up(const Vector2i& pos, int btn);
private:
  Viewer (const Viewer&);
  Viewer& operator= (const Viewer&);
};

#endif

/* EOF */
