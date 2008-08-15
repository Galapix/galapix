/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <iostream>
#include "math/rgb.hpp"
#include "framebuffer.hpp"
#include "software_surface.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"
#include "workspace.hpp"
#include "viewer.hpp"

ViewerState::ViewerState()
  : scale(1.0f),
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
ViewerState::move(const Vector2i& pos)
{
  offset.x += pos.x;
  offset.y += pos.y;
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
    force_redraw(false),
    drag_n_drop(false),
    zoom_button(0)
{
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

            default:
              // ignore all other keypresses
              break;
          }
        break;

      case SDL_MOUSEMOTION:
        mouse_pos = Vector2i(event.motion.x,
                             event.motion.y);
        
        if (drag_n_drop)
          {
            state.move(Vector2i(event.motion.xrel * 4,
                                event.motion.yrel * 4));
          }
        break;


      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
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

  glTranslatef(state.get_offset().x, state.get_offset().y, 0.0f);
  glScalef(state.get_scale(), state.get_scale(), 1.0f);

  Rectf cliprect = state.screen2world(Rect(0, 0, Framebuffer::get_width(), Framebuffer::get_height())); 

  if (clip_debug)
    Framebuffer::draw_rect(cliprect, RGB(255, 0, 255));
  
  workspace.draw(cliprect,
                 state.get_scale());

  glPopMatrix();
}

void
Viewer::update(float delta)
{
  if (zoom_button == -1)
    {
      state.zoom(1.0f / (1.0f + 4.0f * delta), mouse_pos);
    }
  else if (zoom_button == 1)
    {
      state.zoom(1.0f + 4.0f * delta, mouse_pos);
    }
}

/* EOF */
