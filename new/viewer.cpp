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
#include "framebuffer.hpp"
#include "software_surface.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"
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
ViewerState::screen2world(const Rect&) const
{
  return Rectf();
}

Viewer::Viewer()
  : quit(false),
    force_redraw(false),
    drag_n_drop(false),
    surface(SoftwareSurface("test.jpg"))
{
}

void
Viewer::process_event(const SDL_Event& event)
{
  switch(event.type)
    {
      case SDL_QUIT:
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
            state.move(Vector2i(event.motion.xrel, event.motion.yrel));
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
              std::cout << state.screen2world(mouse_pos) << std::endl;

              drag_n_drop = event.button.state;
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
  glPushMatrix();
  
  glTranslatef(state.get_offset().x, state.get_offset().y, 0.0f);
  glScalef(state.get_scale(), state.get_scale(), 1.0f);

  surface.draw(Vector2f(0, 0));

  glPopMatrix();
}

void
Viewer::update(float delta)
{
  if (zoom_button == -1)
    {
      state.zoom(1.0f / (1.0f + 2.0f * delta), mouse_pos);
    }
  else if (zoom_button == 1)
    {
      state.zoom(1.0f + 2.0f * delta, mouse_pos);
    }
}

/* EOF */
