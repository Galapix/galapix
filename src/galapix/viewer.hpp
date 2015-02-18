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

#ifndef HEADER_GALAPIX_GALAPIX_VIEWER_HPP
#define HEADER_GALAPIX_GALAPIX_VIEWER_HPP

#include <memory>
#include <memory>
#include <vector>

#include "display/surface.hpp"
#include "galapix/viewer_state.hpp"
#include "math/rgba.hpp"
#include "math/vector2f.hpp"
#include "math/vector2i.hpp"

class GridTool;
class MoveTool;
class PanTool;
class ResizeTool;
class RotateTool;
class System;
class Tool;
class ViewRotateTool;
class Workspace;
class ZoomRectTool;
class ZoomTool;

enum class MouseButton
{
  LEFT=1, MIDDLE=2, RIGHT=3
};

class Viewer
{
private:
  static Viewer* current_;

public:
  static Viewer* current() { return current_; }

private:
  System& m_system;
  Workspace* m_workspace;
  bool  m_mark_for_redraw;
  bool  m_draw_grid;
  bool  m_pin_grid;
  float m_gamma;
  float m_brightness;
  float m_contrast;

  ViewerState m_state;

  std::shared_ptr<ZoomTool>   keyboard_zoom_in_tool;
  std::shared_ptr<ZoomTool>   keyboard_zoom_out_tool;
  std::shared_ptr<ViewRotateTool> keyboard_view_rotate_tool;

  std::shared_ptr<PanTool>      pan_tool;
  std::shared_ptr<MoveTool>     move_tool;
  std::shared_ptr<ZoomRectTool> zoom_rect_tool;
  std::shared_ptr<ZoomTool>     zoom_in_tool;
  std::shared_ptr<ZoomTool>     zoom_out_tool;
  std::shared_ptr<ResizeTool>   resize_tool;
  std::shared_ptr<RotateTool>   rotate_tool;
  std::shared_ptr<GridTool>     grid_tool;

  Tool* left_tool;
  Tool* middle_tool;
  Tool* right_tool;

  Vector2i m_mouse_pos;

  int m_background_color;
  std::vector<RGBA> m_background_colors;

  Vector2f m_grid_offset;
  Sizef    m_grid_size;
  RGBA     m_grid_color;

public:
  Viewer(System& system, Workspace* workspace);

  void draw();
  void update(float delta);

  void redraw();

  ViewerState& get_state() { return m_state; }
  Workspace*   get_workspace() { return m_workspace; }

  void on_key_up(int key);
  void on_key_down(int key);

  void on_mouse_motion(const Vector2i& pos, const Vector2i& rel);
  void on_mouse_button_down(const Vector2i& pos, MouseButton btn);
  void on_mouse_button_up(const Vector2i& pos, MouseButton btn);

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
  void layout_spiral();
  void layout_vertical();

  void delete_selection();
  void toggle_trackball_mode();
  bool get_trackball_mode() const;

  void load();
  void save();

  void refresh_selection();

  void clear_cache();
  void cleanup_cache();

  void sort_image_list();
  void sort_reverse_image_list();
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
