/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "tools/grid_tool.hpp"

#include "display/framebuffer.hpp"
#include "galapix/viewer.hpp"
#include "math/rgb.hpp"

GridTool::GridTool(Viewer* viewer_)
  : Tool(viewer_),
    mouse_pos(),
    drag_active(false),
    click_pos()
{
}

void
GridTool::move(const Vector2i& pos, const Vector2i& rel)
{
  mouse_pos = pos;
}

void
GridTool::up  (const Vector2i& pos)
{
  if (drag_active)
  {
    drag_active = false;
    //Rect rect(click_pos, mouse_pos);
    //rect.normalize();

    Rectf rect(viewer->get_state().screen2world(click_pos),
               viewer->get_state().screen2world(mouse_pos));
    rect.normalize();

    viewer->set_grid(Vector2f(rect.left, rect.top),
                     rect.get_size());
  }
}

void
GridTool::down(const Vector2i& pos)
{
  click_pos = pos;
  drag_active = true;
}

void
GridTool::draw()
{
  if (drag_active)
  {
    if (1)
    {
      Rectf rect(viewer->get_state().screen2world(click_pos),
                 viewer->get_state().screen2world(mouse_pos));
      rect.normalize();
      Framebuffer::draw_rect(rect, RGB(255, 255, 255));
    }
    else
    {
      // Draw a preview of the grid
      // FIXME: Doesn't work, wrong space
      Rectf rect(viewer->get_state().screen2world(click_pos),
                 viewer->get_state().screen2world(mouse_pos));
      rect.normalize();

      Framebuffer::draw_grid(Vector2f(rect.left, rect.top), rect.get_size(), RGBA(255, 255, 0, 255));
    }
  }
}

/* EOF */
