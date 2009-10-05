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
#include "display/surface.hpp"
#include "math/vector2i.hpp"
#include "math/vector2f.hpp"
#include "math/rgba.hpp"
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
class GridTool;

class Viewer
{
private:
  static Viewer* current_;
 
public:
  static Viewer* current() { return current_; }

private:
  Workspace* workspace;
  bool  mark_for_redraw;
  bool  draw_grid;
  bool  pin_grid;
  float gamma;
  float brightness;
  float contrast;

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
  boost::shared_ptr<GridTool>   grid_tool;

  Tool* left_tool;
  Tool* middle_tool;
  Tool* right_tool;

  Vector2i mouse_pos;

  int background_color;
  std::vector<RGBA> background_colors;

  Vector2f grid_offset;
  Sizef    grid_size;
  RGBA     grid_color;

public:
  Viewer(Workspace* workspace);

  void draw();
  void update(float delta);

  void redraw();

  ViewerState& get_state() { return state; }
  Workspace*   get_workspace() { return workspace; }

  void on_key_up(int key);
  void on_key_down(int key);

  void on_mouse_motion(const Vector2i& pos, const Vector2i& rel);
  void on_mouse_button_down(const Vector2i& pos, int btn);
  void on_mouse_button_up(const Vector2i& pos, int btn);

  bool is_active() const;

  void set_grid(const Vector2f& offset, const Sizef& size);

  // Tool Controls
  void set_pan_tool();
  void set_zoom_tool();
  void set_grid_tool();
  void set_move_resize_tool();
  void set_move_rotate_tool();

  // Gamma Controls
  void increase_contrast();
  void decrease_contrast();

  void increase_brightness();
  void decrease_brightness();

  void increase_gamma();
  void decrease_gamma();

  void reset_gamma();

  // View Controls
  void zoom_to_selection();
  void zoom_home();
  void rotate_view_90();
  void rotate_view_270();
  void reset_view_rotation();

  // Other stuff
  void toggle_grid();
  void toggle_pinned_grid();
  
  void toggle_background_color(bool backwards = false);

  void layout_auto();
  void layout_random();
  void layout_solve_overlaps();
  void layout_tight();
  void layout_vertical();

  void delete_selection();
  void toggle_trackball_mode();

  void load();
  void save();

  void refresh_selection();

  void clear_cache();
  void cleanup_cache();
  void build_quad_tree();
  void clear_quad_tree();

  void sort_image_list();
  void shuffle_image_list();
  
  void isolate_selection();

  void print_images();
  void print_info();
  void print_state();

private:
  Viewer (const Viewer&);
  Viewer& operator= (const Viewer&);
};

#endif

/* EOF */
