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
#include "../framebuffer.hpp"
#include "../viewer.hpp"
#include "../math/rgb.hpp"
#include "zoom_rect_tool.hpp"

ZoomRectTool::ZoomRectTool(Viewer* viewer_)
  : Tool(viewer_),
    drag_active(false)    
{
}

void
ZoomRectTool::move(const Vector2i& pos, const Vector2i& rel)
{
  mouse_pos = pos;
}

void
ZoomRectTool::up  (const Vector2i& pos)
{
  if (drag_active)
    {
      drag_active = false;
      Rectf rect(click_pos,
                 viewer->get_state().screen2world(mouse_pos));
      rect.normalize();

      viewer->get_state().zoom_to(Framebuffer::get_size(), rect);
      std::cout << "Zooming to: " << rect << std::endl;
    }
}

void
ZoomRectTool::down(const Vector2i& pos)
{
  click_pos = viewer->get_state().screen2world(pos);
  drag_active = true;
}

void
ZoomRectTool::draw()
{
  if (drag_active)
    {
      Rectf rect(click_pos,
                 viewer->get_state().screen2world(mouse_pos));
      rect.normalize();
      Framebuffer::draw_rect(rect, RGB(255, 255, 255));
    }
}

/* EOF */
