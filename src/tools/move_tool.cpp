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

#include "tools/move_tool.hpp"

#include "galapix/workspace.hpp"
#include "galapix/viewer.hpp"
#include "display/framebuffer.hpp"
#include "math/rgb.hpp"

MoveTool::MoveTool(Viewer* viewer_)
  : Tool(viewer_),
    mouse_pos(),
    drag_active(false),
    move_active(false),
    click_pos()
{
}

MoveTool::~MoveTool()
{
}

void
MoveTool::move(const Vector2i& pos, const Vector2i& rel)
{
  mouse_pos = pos;

  if (move_active)
  {
    // FIXME: Why does (Vector2i * float) work instead of giving an error?
    viewer->get_workspace()->move_selection(Vector2f(rel) * (1.0f/viewer->get_state().get_scale()));
  }
}

void
MoveTool::up(const Vector2i& /*pos*/)
{
  if (drag_active)
  {
    drag_active = false;
    Rectf rect(click_pos,
               viewer->get_state().screen2world(mouse_pos));
    rect.normalize();
    viewer->get_workspace()->select_images(viewer->get_workspace()->get_images(rect));
  }
  else if (move_active)
  {
    move_active = false;
  }
}

void
MoveTool::down(const Vector2i& pos)
{
  click_pos = viewer->get_state().screen2world(pos);

  ImagePtr image = viewer->get_workspace()->get_image(click_pos);

  if (image)
  {
    // FIXME: add shift/ctrl modifier
    if (viewer->get_workspace()->get_selection()->has(image))
    {
      viewer->get_workspace()->get_selection()->remove_image(image);
    }
    else
    {
      viewer->get_workspace()->get_selection()->add_image(image);
    }
    move_active = true;
  }
  else
  {
    drag_active = true;
    viewer->get_workspace()->clear_selection();
  }
}

void
MoveTool::draw()
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
