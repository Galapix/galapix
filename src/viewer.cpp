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

#include "SDL.h"
#include <fstream>
#include <boost/format.hpp>
#include <iostream>
#include "math/rgb.hpp"
#include "framebuffer.hpp"
#include "sdl_framebuffer.hpp"
#include "software_surface.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"
#include "math/rgba.hpp"
#include "workspace.hpp"
#include "tools/pan_tool.hpp"
#include "tools/move_tool.hpp"
#include "tools/zoom_tool.hpp"
#include "tools/zoom_rect_tool.hpp"
#include "tools/resize_tool.hpp"
#include "tools/rotate_tool.hpp"
#include "tools/grid_tool.hpp"
#include "tools/view_rotate_tool.hpp"
#include "filesystem.hpp"
#include "viewer.hpp"

Viewer* Viewer::current_ = 0;

Viewer::Viewer(Workspace* workspace)
  : workspace(workspace),
    mark_for_redraw(false),
    draw_grid(false),
    pin_grid(false),
    gamma(1.0f),
    brightness(0.0f),
    contrast(1.0f),
    grid_offset(0.0f, 0.0f),
    grid_size(400.0f, 300.0f),
    grid_color(255, 0, 0, 255)
{
  current_ = this;

  pan_tool       = boost::shared_ptr<PanTool>(new PanTool(this));
  move_tool      = boost::shared_ptr<MoveTool>(new MoveTool(this));
  zoom_rect_tool = boost::shared_ptr<ZoomRectTool>(new ZoomRectTool(this));
  resize_tool    = boost::shared_ptr<ResizeTool>(new ResizeTool(this));
  rotate_tool    = boost::shared_ptr<RotateTool>(new RotateTool(this));
  grid_tool      = boost::shared_ptr<GridTool>(new GridTool(this));

  zoom_in_tool  = boost::shared_ptr<ZoomTool>(new ZoomTool(this, -4.0f));
  zoom_out_tool = boost::shared_ptr<ZoomTool>(new ZoomTool(this,  4.0f));

  keyboard_zoom_in_tool  = boost::shared_ptr<ZoomTool>(new ZoomTool(this, -4.0f));
  keyboard_zoom_out_tool = boost::shared_ptr<ZoomTool>(new ZoomTool(this,  4.0f));

  keyboard_view_rotate_tool = boost::shared_ptr<ViewRotateTool>(new ViewRotateTool(this));

  left_tool   = zoom_in_tool.get();
  middle_tool = pan_tool.get();
  right_tool  = zoom_out_tool.get();

  background_color = 0;
  // Black to White
  background_colors.push_back(RGBA(  0,   0,   0));
  background_colors.push_back(RGBA( 64,  64,  64));
  background_colors.push_back(RGBA(128, 128, 128));
  background_colors.push_back(RGBA(255, 255, 255));
  // Rainbow
  background_colors.push_back(RGBA(255,   0,   0));
  background_colors.push_back(RGBA(255, 255,   0));
  background_colors.push_back(RGBA(255,   0, 255));
  background_colors.push_back(RGBA(  0, 255,   0));
  background_colors.push_back(RGBA(  0, 255, 255));
  background_colors.push_back(RGBA(  0,   0, 255));
  // Dimmed Rainbow
  background_colors.push_back(RGBA(128,   0,   0));
  background_colors.push_back(RGBA(128, 128,   0));
  background_colors.push_back(RGBA(128,   0, 128));
  background_colors.push_back(RGBA(  0, 128,   0));
  background_colors.push_back(RGBA(  0, 128, 128));
  background_colors.push_back(RGBA(  0,   0, 128));
}

void
Viewer::redraw()
{
  if (!mark_for_redraw)
    {
      mark_for_redraw = true;
  
#ifdef GALAPIX_SDL
      SDL_Event event;
      event.type = SDL_USEREVENT;
      event.user.code  = 1;
      event.user.data1 = 0;
      event.user.data2 = 0;
  
      while (SDL_PushEvent(&event) != 0);
#endif
    }
}

void
Viewer::draw()
{
  mark_for_redraw = false;
  Framebuffer::clear(background_colors[background_color]);

  bool clip_debug = false;

  glPushMatrix();

  if (clip_debug)
    {
      glTranslatef(Framebuffer::get_width()/2, Framebuffer::get_height()/2, 0.0f);
      glScalef(0.5f, 0.5f, 1.0f);
      glTranslatef(-Framebuffer::get_width()/2, -Framebuffer::get_height()/2, 0.0f);
    }

  Rectf cliprect = state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height())); 

  if (state.get_angle() != 0.0f)
    {
      glTranslatef(Framebuffer::get_width()/2, Framebuffer::get_height()/2, 0.0f);
      glRotatef(state.get_angle(), 0.0f, 0.0f, 1.0f); // Rotates around 0.0
      glTranslatef(-Framebuffer::get_width()/2, -Framebuffer::get_height()/2, 0.0f);

      // FIXME: We enlarge the cliprect so much that we can rotate
      // freely, however this enlargement creates a cliprect that
      // might be quite a bit larger then what is really needed
      float  diagonal = cliprect.get_diagonal();
      Vector2f center = cliprect.get_center();
      cliprect.left   = center.x - diagonal;
      cliprect.right  = center.x + diagonal;
      cliprect.top    = center.y - diagonal;
      cliprect.bottom = center.y + diagonal;
    }

  glTranslatef(state.get_offset().x, state.get_offset().y, 0.0f);
  glScalef(state.get_scale(), state.get_scale(), 1.0f);

  if (clip_debug)
    Framebuffer::draw_rect(cliprect, RGB(255, 0, 255));
  
  workspace->draw(cliprect,
                  state.get_scale());

  left_tool->draw();
  middle_tool->draw();
  right_tool->draw();

  glPopMatrix();

  if (draw_grid)
    {
      if (pin_grid)
        {
          Framebuffer::draw_grid(grid_offset * state.get_scale() + state.get_offset(), 
                                 grid_size * state.get_scale(),
                                 grid_color);
        }
      else
        {
          Framebuffer::draw_grid(grid_offset, grid_size, grid_color);
        }
    }
}

void
Viewer::update(float delta)
{
  workspace->update(delta);

  zoom_in_tool ->update(mouse_pos, delta);
  zoom_out_tool->update(mouse_pos, delta);

  keyboard_zoom_in_tool ->update(mouse_pos, delta);
  keyboard_zoom_out_tool->update(mouse_pos, delta);
}

void
Viewer::on_mouse_motion(const Vector2i& pos, const Vector2i& rel)
{
  mouse_pos = pos;

  left_tool  ->move(mouse_pos, rel);
  middle_tool->move(mouse_pos, rel);
  right_tool ->move(mouse_pos, rel);

  keyboard_view_rotate_tool->move(mouse_pos, rel);
}

void
Viewer::on_mouse_button_down(const Vector2i& pos, int btn)
{
  mouse_pos = pos;

  switch(btn)
    {
      case SDL_BUTTON_LEFT:
        left_tool->down(pos);
        break;

      case SDL_BUTTON_RIGHT:
        right_tool->down(pos);
        break;

      case SDL_BUTTON_MIDDLE:
        middle_tool->down(pos);
        break;
    }
}

void
Viewer::on_mouse_button_up(const Vector2i& pos, int btn)
{
  mouse_pos = pos;

  switch(btn)
    {
      case SDL_BUTTON_LEFT:
        left_tool->up(pos);
        break;

      case SDL_BUTTON_RIGHT:
        right_tool->up(pos);
        break;

      case SDL_BUTTON_MIDDLE:
        middle_tool->up(pos);
        break;
    }
}

void
Viewer::on_key_up(int key)
{
  switch(key)
    {
      case SDLK_END:
        keyboard_zoom_out_tool->up(mouse_pos);
        break;

      case SDLK_HOME:
        keyboard_zoom_in_tool->up(mouse_pos);
        break;

      case SDLK_RSHIFT:
      case SDLK_LSHIFT:
        keyboard_view_rotate_tool->up(mouse_pos);
        break;
    }
}

void
Viewer::on_key_down(int key)
{
  switch(key)
    {
      case SDLK_END:
        keyboard_zoom_out_tool->down(mouse_pos);
        break;

      case SDLK_HOME:
        keyboard_zoom_in_tool->down(mouse_pos);
        break;

      case SDLK_RSHIFT:
      case SDLK_LSHIFT:
        keyboard_view_rotate_tool->down(mouse_pos);
        break;
    }
}

bool
Viewer::is_active() const
{
  return
    workspace->is_animated()   ||
    zoom_in_tool ->is_active() ||
    zoom_out_tool->is_active() ||
    keyboard_zoom_in_tool ->is_active() ||
    keyboard_zoom_out_tool->is_active();
}

void
Viewer::set_grid(const Vector2f& offset, const Sizef& size)
{
  if (pin_grid)
    {
      grid_offset = offset;
      grid_size   = size;
    }
  else
    {
      grid_offset = offset * state.get_scale() + state.get_offset();
      grid_size   = size * state.get_scale();
    }
}

void
Viewer::set_pan_tool()
{
  std::cout << "Pan&Zoom Tools selected" << std::endl;
  left_tool   = zoom_in_tool.get();
  right_tool  = zoom_out_tool.get();              
  middle_tool = pan_tool.get();
}

void
Viewer::set_zoom_tool()
{
  std::cout << "Zoom&Pan Tools selected" << std::endl;
  left_tool   = zoom_rect_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_grid_tool()
{
  std::cout << "Grid Tool selected" << std::endl;
  left_tool   = grid_tool.get();
  right_tool  = zoom_out_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::set_move_resize_tool()
{
  std::cout << "Move&Resize Tools selected" << std::endl;
  left_tool   = move_tool.get();
  right_tool  = resize_tool.get();              
  middle_tool = pan_tool.get();
}

void
Viewer::set_move_rotate_tool()
{
  std::cout << "Move&Rotate Tools selected" << std::endl;
  left_tool   = move_tool.get();
  right_tool  = rotate_tool.get();
  middle_tool = pan_tool.get();
}

void
Viewer::increase_contrast()
{
  //contrast += 0.1f;
  contrast *= 1.1f;
  std::cout << "Contrast: " << contrast << std::endl;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::decrease_contrast()
{
  //contrast -= 0.1f;
  contrast /= 1.1f;
  std::cout << "Contrast: " << contrast << std::endl;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::increase_brightness()
{
  brightness += 0.1f;
  std::cout << "Brightness: " << brightness << std::endl;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::decrease_brightness()
{
  brightness -= 0.1f;
  std::cout << "Brightness: " << brightness << std::endl;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::increase_gamma()
{
  gamma *= 1.1f;
  std::cout << "Gamma: " << gamma << std::endl;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::decrease_gamma()
{
  gamma /= 1.1f;
  std::cout << "Gamma: " << gamma << std::endl;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::reset_gamma()
{
  brightness = 0.0f;
  contrast   = 1.0f;
  gamma      = 1.0f;
#ifdef GALAPIX_SDL
  SDLFramebuffer::apply_gamma_ramp(contrast, brightness, gamma);
#endif
}

void
Viewer::toggle_grid()
{
  draw_grid = !draw_grid;
  std::cout << "Draw Grid: " << draw_grid << std::endl;
}

void
Viewer::layout_auto()
{
  workspace->layout_aspect(Framebuffer::get_width(), Framebuffer::get_height());
}

void
Viewer::layout_random()
{
  workspace->layout_random();
}

void
Viewer::layout_solve_overlaps()
{
  workspace->solve_overlaps();
}

void
Viewer::layout_tight()
{
  workspace->layout_tight(Framebuffer::get_width(), Framebuffer::get_height());
}

void
Viewer::layout_vertical()
{
  workspace->layout_vertical();
}

void
Viewer::toggle_pinned_grid()
{
  pin_grid = !pin_grid;
  std::cout << "Pin Grid: " << pin_grid << std::endl;
  if (!pin_grid)
    {
      grid_offset = grid_offset * state.get_scale() + state.get_offset();
      grid_size  *= state.get_scale();
    }
  else
    {
      grid_offset = (grid_offset - state.get_offset()) / state.get_scale();
      grid_size  /= state.get_scale();            
    }
}

void
Viewer::toggle_background_color()
{
  background_color += 1;
  if (background_color >= int(background_colors.size()))
    background_color = 0;
}

void
Viewer::zoom_home()
{
  state.set_offset(Vector2f(0.0f, 0.0f));
  state.set_angle(0.0f);
  state.set_scale(1.0f);
}

void
Viewer::zoom_to_selection()
{
  if (!workspace->get_selection().empty())
    {
      state.zoom_to(Framebuffer::get_size(),
                    workspace->get_selection().get_bounding_rect());
    }
  else
    {
      state.zoom_to(Framebuffer::get_size(),
                    workspace->get_bounding_rect());
    }
}

void 
Viewer::rotate_view_90()
{
  state.rotate(90.0f);
}

void 
Viewer::rotate_view_270()
{
  state.rotate(-90.0f);
}

void
Viewer::delete_selection()
{
  workspace->delete_selection();
}

void
Viewer::reset_view_rotation()
{
  state.set_angle(0.0f);
}

void
Viewer::toggle_trackball_mode()
{
  pan_tool->set_trackball_mode(!pan_tool->get_trackball_mode());
  if (pan_tool->get_trackball_mode())
    {
      std::cout << "Trackball mode active, press 't' to leave" << std::endl;
      SDL_ShowCursor(SDL_DISABLE);
      SDL_WM_GrabInput(SDL_GRAB_ON);
    }
  else
    {
      std::cout << "Trackball mode deactivated" << std::endl;
      SDL_ShowCursor(SDL_ENABLE);
      SDL_WM_GrabInput(SDL_GRAB_OFF);
    }
}

void
Viewer::load()
{
  workspace->load("/tmp/workspace-dump.galapix");
}

void
Viewer::save()
{
  std::ofstream out("/tmp/workspace-dump.galapix");
  workspace->save(out);
  out.close();
}

void
Viewer::refresh_selection()
{
  // FIXME: Make force on Shift-F5 and normal F5 only refresh if the file changed
  std::cout << "Refreshing tiles..." << std::endl;
  Selection selection = workspace->get_selection();
  bool force = true; // FIXME: keystates[SDLK_RSHIFT] || keystates[SDLK_LSHIFT];
  for(Selection::iterator i = selection.begin(); i != selection.end(); ++i)
    {
      i->refresh(force);
    }
}

void
Viewer::clear_cache()
{
  std::cout << "Workspace: Clearing cache" << std::endl;
  workspace->clear_cache();
}

void
Viewer::cleanup_cache()
{
  std::cout << "Workspace: Cache Cleanup" << std::endl;
  workspace->cache_cleanup();
}

void
Viewer::build_quad_tree()
{
  std::cout << "Building QuadTree... " << std::flush;
  workspace->build_quad_tree();
  std::cout << "done" << std::endl;
}

void
Viewer::clear_quad_tree()
{
  std::cout << "Killing QuadTree" << std::endl;
  workspace->clear_quad_tree();                
}

void
Viewer::sort_image_list()
{
  std::cout << "Workspace: Sorting" << std::endl;
  workspace->sort();
}

void
Viewer::shuffle_image_list()
{
  std::cout << "Workspace: Random Shuffle" << std::endl;
  workspace->random_shuffle();
}

void
Viewer::isolate_selection()
{
  workspace->isolate_selection();
}

void
Viewer::print_images()
{
  Rectf cliprect = state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height()));
  workspace->print_images(cliprect);
}

void
Viewer::print_info()
{
  workspace->print_info();
}

void
Viewer::print_state()
{
  std::cout << state.get_offset() << " " << state.get_scale() << std::endl;
}

/* EOF */
