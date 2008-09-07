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

#include "image.hpp"
#include "workspace.hpp"
#include "viewer.hpp"
#include "framebuffer.hpp"
#include "math/rgb.hpp"
#include "move_tool.hpp"

MoveTool::MoveTool(Viewer* viewer)
  : Tool(viewer),
    drag_active(false),
    move_active(false)
{
}

MoveTool::~MoveTool()
{
}

void
MoveTool::mouse_move(const Vector2i& pos, const Vector2i& rel)
{
  mouse_pos = pos;

  if (move_active)
    {
      for(std::vector<Image>::iterator i = selected_images.begin(); i != selected_images.end(); ++i)
        {
          i->set_pos(i->get_pos() + rel * (1.0f/viewer->get_state().get_scale()));
        }
    }
}

void
MoveTool::mouse_btn_up  (int num, const Vector2i& pos)
{
  switch (num)
    {
      case SDL_BUTTON_LEFT:
        {
          if (drag_active)
            {
              drag_active = false;
              Rectf rect(click_pos,
                         viewer->get_state().screen2world(mouse_pos));
              rect.normalize();
              selected_images = viewer->get_workspace()->get_images(rect);
            }
          else if (move_active)
            {
              move_active = false;
            }
        }
        break;
        
      default:
        break;
    }
}

void
MoveTool::mouse_btn_down(int num, const Vector2i& pos)
{
  switch (num)
    {
      case SDL_BUTTON_LEFT:
        {
          click_pos = viewer->get_state().screen2world(pos);

          if (selection_clicked(click_pos))
            {
              move_active = true;
            }
          else
            {
              drag_active = true;
              selected_images.clear();
            }
        }
        break;
        
      default:
        break;
    }
}

bool
MoveTool::selection_clicked(const Vector2f& pos)
{
  for(std::vector<Image>::iterator i = selected_images.begin(); i != selected_images.end(); ++i)
    {
      if (i->overlaps(pos))
        return true;
    }
  return false;
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
  else
    {
      for(std::vector<Image>::iterator i = selected_images.begin(); i != selected_images.end(); ++i)
        {
          i->draw_mark();
        }
    }
}

void
MoveTool::update(float delta)
{
}

/* EOF */
