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

#include "viewer.hpp"
#include "pan_tool.hpp"

PanTool::PanTool(Viewer* viewer)
  : Tool(viewer),
    trackball_mode(false),
    move_active(false),
    zoom_in(false),
    zoom_out(false)
{
}

PanTool::~PanTool()
{
}

void
PanTool::mouse_move(const Vector2i& pos, const Vector2i& rel)
{
  mouse_pos = pos;

  if (trackball_mode)
    {
      viewer->get_state().move(rel * 4);
    }
  else if (move_active)
    { // FIXME: This is of course wrong, since depending on x/yrel will lead to drift
      // Also we shouldn't use 4x speed, but 1x seems so useless
      viewer->get_state().move(rel * 4);
    }
}

void
PanTool::mouse_btn_up(int num, const Vector2i& pos)
{
  mouse_pos = pos;

  switch (num)
    {
      case SDL_BUTTON_LEFT:
        zoom_out = false;
        break;
        
      case SDL_BUTTON_RIGHT:
        zoom_in = false;
        break;

      case SDL_BUTTON_MIDDLE:
        move_active = false;
        break;

      case SDL_BUTTON_WHEELUP:
        viewer->get_state().zoom(1.1f, mouse_pos);
        break;
        
      case SDL_BUTTON_WHEELDOWN:
        viewer->get_state().zoom(1.0f/1.1f, mouse_pos);
        break;

      default:
        break;
    }
}

void
PanTool::mouse_btn_down(int num, const Vector2i& pos)
{
  mouse_pos = pos;

  switch (num)
    {
      case SDL_BUTTON_LEFT:
        zoom_out = true;
        break;
        
      case SDL_BUTTON_RIGHT:
        zoom_in = true;
        break;

      case SDL_BUTTON_MIDDLE:
        move_active = true;
        break;

      default:
        break;
    }
}

void
PanTool::draw()
{
}

void
PanTool::update(float delta)
{
  if (trackball_mode)
    {
      if (zoom_in && !zoom_out)
        viewer->get_state().zoom(1.0f / (1.0f + 4.0f * delta));
      else if (!zoom_in && zoom_out)
        viewer->get_state().zoom(1.0f + 4.0f * delta);
    }
  else
    {
      if (zoom_in && !zoom_out)
        viewer->get_state().zoom(1.0f / (1.0f + 4.0f * delta), mouse_pos);
      else if (!zoom_in && zoom_out)
        viewer->get_state().zoom(1.0f + 4.0f * delta, mouse_pos);
    }
}

bool
PanTool::get_trackball_mode() const
{
  return trackball_mode;
}

void
PanTool::set_trackball_mode(bool mode)
{
  trackball_mode = mode;
}

/* EOF */
