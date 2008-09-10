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
    resize_active(false),
    old_scale(1.0f),
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
      viewer->get_workspace()->move_selection(rel * (1.0f/viewer->get_state().get_scale()));
    }
  
  if (resize_active)
    {
      Vector2f p = viewer->get_state().screen2world(pos);

      float a = (selection_center - p).length();
      float b = (selection_center - resize_center).length();

      if (b != 0.0f)
        {
          //std::cout << a << " " << b << " " << a / b << std::endl;
          viewer->get_workspace()->scale_selection((1.0f/old_scale) * (a/b));
          old_scale = a/b; // FIXME: Hack, should scale to original scale 
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
              viewer->get_workspace()->select_images(viewer->get_workspace()->get_images(rect));
            }
          else if (move_active)
            {
              move_active = false;
            }
        }
        break;
        
      case SDL_BUTTON_RIGHT:
        resize_active = false;
        old_scale = 1.0f;
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

          if (viewer->get_workspace()->selection_clicked(click_pos))
            {
              move_active = true;
            }
          else
            {
              drag_active = true;
              viewer->get_workspace()->clear_selection();
            }
        }
        break;
       
      case SDL_BUTTON_RIGHT:
        resize_active = true;
        resize_center    = viewer->get_state().screen2world(pos);
        selection_center = viewer->get_workspace()->get_selection().get_center();
        break;
        
      default:
        break;
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
  else
    {
    }
}

void
MoveTool::update(float delta)
{
}

/* EOF */
