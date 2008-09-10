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

#include <iostream>
#include <math.h>
#include "math/rgb.hpp"
#include "framebuffer.hpp"
#include "software_surface.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"
#include "workspace.hpp"
#include "pan_tool.hpp"
#include "move_tool.hpp"
#include "zoom_tool.hpp"
#include "resize_tool.hpp"
#include "rotate_tool.hpp"
#include "viewer.hpp"

Viewer::Viewer(Workspace* workspace)
  : workspace(workspace),
    quit(false),
    force_redraw(false),
    draw_grid(false),
    gamma(1.0f)
{
  pan_tool    = boost::shared_ptr<PanTool>(new PanTool(this));
  move_tool   = boost::shared_ptr<MoveTool>(new MoveTool(this));
  resize_tool = boost::shared_ptr<ResizeTool>(new ResizeTool(this));
  rotate_tool = boost::shared_ptr<RotateTool>(new RotateTool(this));

  zoom_in_tool  = boost::shared_ptr<ZoomTool>(new ZoomTool(this, -4.0f));
  zoom_out_tool = boost::shared_ptr<ZoomTool>(new ZoomTool(this,  4.0f));

  left_tool   = zoom_in_tool.get();
  middle_tool = pan_tool.get();
  right_tool  = zoom_out_tool.get();
}

void
Viewer::process_event(const SDL_Event& event)
{
  switch(event.type)
    {
      case SDL_QUIT:
        std::cout << "Viewer: SDL_QUIT received" << std::endl;
        quit = true;
        break;

      case SDL_VIDEOEXPOSE:
        force_redraw = true;
        break;

      case SDL_VIDEORESIZE:
        Framebuffer::resize(event.resize.w, event.resize.h);
        force_redraw = true;
        break;

      case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
          {
            case SDLK_ESCAPE:
              quit = true;
              break;

            case SDLK_PAGEUP:
              gamma *= 1.1f;
              SDL_SetGamma(gamma, gamma, gamma);
              break;

            case SDLK_PAGEDOWN:
              gamma /= 1.1f;
              SDL_SetGamma(gamma, gamma, gamma);
              break;

            case SDLK_F11:
              Framebuffer::toggle_fullscreen();
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

            case SDLK_UP:
              state.set_angle(0.0f);
              break;

            case SDLK_DOWN:
              state.set_angle(0.0f);
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

            case SDLK_m:
              std::cout << "Move&Resize Tools selected" << std::endl;
              left_tool   = move_tool.get();
              right_tool  = resize_tool.get();              
              middle_tool = pan_tool.get();
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

              //case SDLK_i:
              //workspace->print_info();
              //break;

            case SDLK_g:
              draw_grid = !draw_grid;
              break;

            case SDLK_1:
              workspace->layout(4, 3);
              break;

            case SDLK_2:
              workspace->layout(16, 9);
              break;

            case SDLK_3:
              workspace->random_layout();
              break;

            case SDLK_4:
              workspace->solve_overlaps();
              break;

            default:
              break;
          }
        break;

      case SDL_MOUSEMOTION:
        mouse_pos = Vector2i(event.motion.x, event.motion.y);

        left_tool  ->move(mouse_pos, Vector2i(event.motion.xrel, event.motion.yrel));
        middle_tool->move(mouse_pos, Vector2i(event.motion.xrel, event.motion.yrel));
        right_tool ->move(mouse_pos, Vector2i(event.motion.xrel, event.motion.yrel));
        break;
        
      case SDL_MOUSEBUTTONDOWN:
        mouse_pos = Vector2i(event.button.x, event.button.y);

        switch(event.button.button)
          {
            case SDL_BUTTON_WHEELUP:
              get_state().zoom(1.1f, mouse_pos);
              break;
        
            case SDL_BUTTON_WHEELDOWN:
              get_state().zoom(1.0f/1.1f, mouse_pos);
              break;

            case SDL_BUTTON_LEFT:
              left_tool->down(Vector2i(event.button.x, event.button.y));
              break;

            case SDL_BUTTON_RIGHT:
              right_tool->down(Vector2i(event.button.x, event.button.y));
              break;

            case SDL_BUTTON_MIDDLE:
              middle_tool->down(Vector2i(event.button.x, event.button.y));
              break;
          }
        break;

      case SDL_MOUSEBUTTONUP:
        mouse_pos = Vector2i(event.button.x, event.button.y);

        // FIXME: SDL Reverses the mouse buttons when a grab is active!
        switch(event.button.button)
          {
            case SDL_BUTTON_LEFT:
              left_tool->up(Vector2i(event.button.x, event.button.y));
              break;

            case SDL_BUTTON_RIGHT:
              right_tool->up(Vector2i(event.button.x, event.button.y));
              break;

            case SDL_BUTTON_MIDDLE:
              middle_tool->up(Vector2i(event.button.x, event.button.y));
              break;
          }
        break;

      default:
        break;
    }
}

void
Viewer::draw()
{
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
    Framebuffer::draw_grid(3);
}

void
Viewer::update(float delta)
{
  workspace->update(delta);

  left_tool  ->update(mouse_pos, delta);
  middle_tool->update(mouse_pos, delta);
  right_tool ->update(mouse_pos, delta);
}

/* EOF */
