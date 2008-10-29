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
#include <math.h>
#include "math/rgb.hpp"
#include "framebuffer.hpp"
#include "sdl_framebuffer.hpp"
#include "software_surface.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"
#include "math/rgba.hpp"
#include "workspace.hpp"
#include "pan_tool.hpp"
#include "move_tool.hpp"
#include "zoom_tool.hpp"
#include "zoom_rect_tool.hpp"
#include "resize_tool.hpp"
#include "rotate_tool.hpp"
#include "grid_tool.hpp"
#include "filesystem.hpp"
#include "view_rotate_tool.hpp"
#include "png.hpp"
#include "viewer.hpp"

Viewer::Viewer(Workspace* workspace)
  : workspace(workspace),
    draw_grid(false),
    pin_grid(false),
    gamma(1.0f),
    brightness(0.0f),
    contrast(1.0f),
    grid_offset(0.0f, 0.0f),
    grid_size(400.0f, 300.0f)
{
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
Viewer::draw()
{
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
                                 RGBA(255, 255, 255, 150));
        }
      else
        {
          Framebuffer::draw_grid(grid_offset, grid_size, RGBA(255, 255, 255, 150));
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
      case SDL_BUTTON_WHEELUP:
        get_state().zoom(1.1f, pos);
        break;
        
      case SDL_BUTTON_WHEELDOWN:
        get_state().zoom(1.0f/1.1f, pos);
        break;

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

      default:
        break;
    }
}

void apply_gamma_ramp(float contrast, float brightness, float gamma)
{
  Uint16 tbl[256];
  for(int i = 0; i < 256; ++i)
    {
      float c = i/255.0f;
      c = c + brightness;
      c = (c * contrast) - 0.5f * (contrast - 1.0f);
      c = powf(c, 1.0f/gamma);
      
      tbl[i] = Math::clamp(0, (int)(c*65535.0f), 65535);
    }
  
  SDL_SetGammaRamp(tbl, tbl, tbl);
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

      case SDLK_F6:
        brightness -= 0.1f;
        std::cout << "Brightness: " << brightness << std::endl;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;

      case SDLK_F7:
        brightness += 0.1f;
        std::cout << "Brightness: " << brightness << std::endl;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;        

      case SDLK_F8:
        //contrast -= 0.1f;
        contrast /= 1.1f;
        std::cout << "Contrast: " << contrast << std::endl;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;        

      case SDLK_F9:
        //contrast += 0.1f;
        contrast *= 1.1f;
        std::cout << "Contrast: " << contrast << std::endl;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;        

      case SDLK_F10:
        brightness = 0.0f;
        contrast   = 1.0f;
        gamma      = 1.0f;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;

      case SDLK_PAGEUP:
        gamma *= 1.1f;
        std::cout << "Gamma: " << gamma << std::endl;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;

      case SDLK_PAGEDOWN:
        gamma /= 1.1f;
        std::cout << "Gamma: " << gamma << std::endl;
        apply_gamma_ramp(contrast, brightness, gamma);
        break;

      case SDLK_F11:
        SDLFramebuffer::toggle_fullscreen();
        break;

      case SDLK_LEFT:
        state.rotate(90.0f);
        break;

      case SDLK_RIGHT:
        state.rotate(-90.0f);
        break;

      case SDLK_7:
        workspace->get_selection().scale(1.0f/1.1f);
        break;

      case SDLK_8:
        workspace->get_selection().scale(1.1f);
        break;

      case SDLK_DELETE:
        workspace->delete_selection();
        break;
              
      case SDLK_SPACE:
        {
          Rectf cliprect = state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height()));
          workspace->print_images(cliprect);
        }
        break;

      case SDLK_RSHIFT:
      case SDLK_LSHIFT:
        keyboard_view_rotate_tool->down(mouse_pos);
        break;

      case SDLK_UP:
        state.set_angle(0.0f);
        break;

      case SDLK_DOWN:
        state.set_angle(0.0f);
        break;

      case SDLK_F2:
        workspace->load("/tmp/workspace-dump.galapix");
        break;

      case SDLK_F3:
        {
          std::ofstream out("/tmp/workspace-dump.galapix");
          workspace->save(out);
          out.close();
        }
        break;

      case SDLK_F5:
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
        break;

      case SDLK_q:
        std::cout << "Building QuadTree... " << std::flush;
        workspace->build_quad_tree();
        std::cout << "done" << std::endl;
        break;
              
      case SDLK_j:
        std::cout << "Killing QuadTree" << std::endl;
        workspace->clear_quad_tree();              
        break;
                
      case SDLK_c:
        std::cout << "Workspace: Clearing cache" << std::endl;
        workspace->clear_cache();
        break;

      case SDLK_k:
        std::cout << "Workspace: Cache Cleanup" << std::endl;
        workspace->cache_cleanup();
        break;

      case SDLK_t:
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
        break;

      case SDLK_i:
        workspace->isolate_selection();
        break;

      case SDLK_p:
        std::cout << "Pan&Zoom Tools selected" << std::endl;
        left_tool   = zoom_in_tool.get();
        right_tool  = zoom_out_tool.get();              
        middle_tool = pan_tool.get();
        break;

      case SDLK_r:
        std::cout << "Move&Rotate Tools selected" << std::endl;
        left_tool   = move_tool.get();
        right_tool  = rotate_tool.get();
        middle_tool = pan_tool.get();
        break;

      case SDLK_l:
        std::cout << state.get_offset() << " " << state.get_scale() << std::endl;
        break;

      case SDLK_F12:
        {
          SoftwareSurface surface = Framebuffer::screenshot();
          // FIXME: Could do this in a worker thread to avoid pause on screenshotting
          for(int i = 0; ; ++i)
            {
              std::string outfile = (boost::format("/tmp/galapix-screenshot-%04d.png") % i).str();
              if (!Filesystem::exist(outfile.c_str()))
                {
                  PNG::save(surface, outfile);
                  std::cout << "Screenshot written to " << outfile << std::endl;
                  break;
                }
            }
        }
        break;
        
      case SDLK_z:
        std::cout << "Zoom&Resize Tools selected" << std::endl;
        left_tool   = zoom_rect_tool.get();
        right_tool  = zoom_out_tool.get();
        middle_tool = pan_tool.get();
        break;

      case SDLK_m:
        std::cout << "Move&Resize Tools selected" << std::endl;
        left_tool   = move_tool.get();
        right_tool  = resize_tool.get();              
        middle_tool = pan_tool.get();
        break;

      case SDLK_d:
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
        break;
              
      case SDLK_h:
        state.set_offset(Vector2f(0.0f, 0.0f));
        state.set_angle(0.0f);
        state.set_scale(1.0f);
        break;

      case SDLK_s:
        std::cout << "Workspace: Sorting" << std::endl;
        workspace->sort();
        break;

      case SDLK_n:
        std::cout << "Workspace: Random Shuffle" << std::endl;
        workspace->random_shuffle();
        break;

      case SDLK_0:
        workspace->print_info();
        break;

      case SDLK_b:
        background_color += 1;
        if (background_color >= int(background_colors.size()))
          background_color = 0;
        break;

      case SDLK_f:
        pin_grid = !pin_grid;
        std::cout << "Pin Grid: " << pin_grid << std::endl;
        break;

      case SDLK_g:
        draw_grid = !draw_grid;
        std::cout << "Draw Grid: " << draw_grid << std::endl;
        break;

      case SDLK_y:
        std::cout << "Grid Tool selected" << std::endl;
        left_tool   = grid_tool.get();
        right_tool  = zoom_out_tool.get();
        middle_tool = pan_tool.get();
        break;

      case SDLK_1:
        workspace->layout_aspect(4, 3);
        break;

      case SDLK_2:
        workspace->layout_aspect(16, 9);
        break;

      case SDLK_3:
        workspace->layout_random();
        break;

      case SDLK_4:
        workspace->solve_overlaps();
        break;

      case SDLK_5:
        workspace->layout_tight();
        break;

      case SDLK_6:
        workspace->layout_vertical();
        break;

      default:
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
  grid_offset = offset;
  grid_size   = size;
}

/* EOF */
