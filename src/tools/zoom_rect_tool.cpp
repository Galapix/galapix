// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "tools/zoom_rect_tool.hpp"

#include <iostream>

#include "display/framebuffer.hpp"
#include "galapix/viewer.hpp"
#include "math/rgb.hpp"

ZoomRectTool::ZoomRectTool(Viewer* viewer_)
  : Tool(viewer_),
    mouse_pos(),
    drag_active(false),
    click_pos()
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
