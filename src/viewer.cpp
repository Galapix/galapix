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
#include "viewer.hpp"

ViewerState::ViewerState()
  : scale(0.25f),
    angle(0.0f),
    offset(0.0f, 0.0f)
{
}

void
ViewerState::zoom(float factor, const Vector2i& pos)
{
  scale *= factor;

  offset = Vector2f(pos) - ((Vector2f(pos) - offset) * factor);
}

void
ViewerState::zoom(float factor)
{
  zoom(factor, Vector2i(Framebuffer::get_width()/2, // FIXME: Little ugly, isn't it?
                        Framebuffer::get_height()/2));
}

void
ViewerState::rotate(float r)
{
  angle += r;
}

void
ViewerState::set_angle(float r)
{
  angle = r;
}

void
ViewerState::move(const Vector2f& pos)
{
  // FIXME: Implement a proper 2D Matrix instead of this hackery
  offset.x += pos.x * cosf(angle/180.0f*M_PI) +  pos.y * sinf(angle/180.0f*M_PI);
  offset.y -= pos.x * sinf(angle/180.0f*M_PI) -  pos.y * cosf(angle/180.0f*M_PI);
}

Vector2f
ViewerState::screen2world(const Vector2i& pos) const
{
  return (Vector2f(pos) - offset) / scale;
}

Rectf
ViewerState::screen2world(const Rect& rect) const
{
  return Rectf((rect.left   - offset.x) / scale,
               (rect.top    - offset.y) / scale,
               (rect.right  - offset.x) / scale,
               (rect.bottom - offset.y) / scale);
}

Viewer::Viewer()
  : quit(false),
    trackball_mode(false),
    force_redraw(false),
    drag_n_drop(false),
    draw_grid(false),
    zoom_button(0),
    gamma(1.0f)
{
}

void
Viewer::process_event(Workspace& workspace, const SDL_Event& event)
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

            case SDLK_UP:
              state.set_angle(0.0f);
              break;

            case SDLK_DOWN:
              state.set_angle(0.0f);
              break;
                
            case SDLK_c:
              std::cout << "Workspace: Clearing cache" << std::endl;
              workspace.clear_cache();
              break;

            case SDLK_k:
              std::cout << "Workspace: Cache Cleanup" << std::endl;
              workspace.cache_cleanup();
              break;

            case SDLK_t:
              trackball_mode = !trackball_mode;
              if (trackball_mode)
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

            case SDLK_s:
              workspace.sort();
              break;

            case SDLK_i:
              workspace.print_info();
              break;

            case SDLK_g:
              draw_grid = !draw_grid;
              break;

            case SDLK_1:
              workspace.layout(4, 3);
              break;

            case SDLK_2:
              workspace.layout(16, 9);
              break;

            default:
              break;
          }
        break;

      case SDL_MOUSEMOTION:
        if (trackball_mode)
          {
            state.move(Vector2f(event.motion.xrel * 4.0f,
                                event.motion.yrel * 4.0f));
          }
        else
          {
            mouse_pos = Vector2i(event.motion.x,
                                 event.motion.y);
        
            if (drag_n_drop)
              { // FIXME: This is of course wrong, since depending on x/yrel will lead to drift
                // Also we shouldn't use 4x speed, but 1x seems so useless
                state.move(Vector2f(event.motion.xrel * 4,
                                    event.motion.yrel * 4));
              }
          }
        break;


      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        // FIXME: SDL Reverses the mouse buttons when a grab is active!
        switch(event.button.button)
          {
            case SDL_BUTTON_WHEELUP:
              if (event.button.state == SDL_PRESSED)
                {
                  state.zoom(1.1f, mouse_pos);
                }
              break;

            case SDL_BUTTON_WHEELDOWN:
              if (event.button.state == SDL_PRESSED)
                {
                  state.zoom(1.0f/1.1f, mouse_pos);
                }
              break;
                  
            case SDL_BUTTON_LEFT:
              if (event.button.state == SDL_PRESSED)
                zoom_button = 1;
              else
                zoom_button = 0;
              break;

            case SDL_BUTTON_RIGHT:
              if (event.button.state == SDL_PRESSED)
                zoom_button = -1;
              else
                zoom_button = 0;
              break;
  
            case SDL_BUTTON_MIDDLE:
              //std::cout << state.screen2world(mouse_pos) << std::endl;
              if (trackball_mode)
                drag_n_drop = false;
              else
                drag_n_drop = event.button.state;
              break;
          }
        break;

      default:
        break;
    }
}

void
Viewer::draw(Workspace& workspace)
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
  
  workspace.draw(cliprect,
                 state.get_scale());

  glPopMatrix();

  if (draw_grid)
    Framebuffer::draw_grid(3);
}

void
Viewer::update(Workspace& workspace, float delta)
{
  if (trackball_mode)
    {
      if (zoom_button == -1)
        state.zoom(1.0f / (1.0f + 4.0f * delta));
      else if (zoom_button == 1)
        state.zoom(1.0f + 4.0f * delta);
    }
  else
    {
      if (zoom_button == -1)
        state.zoom(1.0f / (1.0f + 4.0f * delta), mouse_pos);
      else if (zoom_button == 1)
        state.zoom(1.0f + 4.0f * delta, mouse_pos);
    }

  workspace.update(delta);
}

/* EOF */
