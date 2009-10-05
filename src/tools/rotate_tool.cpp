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

#include <math.h>
#include "../viewer.hpp"
#include "../workspace.hpp"
#include "rotate_tool.hpp"

RotateTool::RotateTool(Viewer* viewer_)
  : Tool(viewer_),
    rotate_active(false),
    start_angle(0.0f)
{
}

void
RotateTool::move(const Vector2i& pos, const Vector2i& /*rel*/)
{
  if (rotate_active)
    {
      Vector2f mouse_pos = viewer->get_state().screen2world(pos);
      float angle = atan2f(selection_center.y - mouse_pos.y,
                           selection_center.x - mouse_pos.x);  

      std::cout << "Angle: " << ((start_angle - angle)/M_PI*180.0f) << std::endl;

      Selection selection = viewer->get_workspace()->get_selection();
      for(Selection::iterator i = selection.begin(); i != selection.end(); ++i)
        {
          i->set_angle(start_angle - angle);
        }
    }
}

void
RotateTool::up  (const Vector2i& /*pos*/)
{
  rotate_active = false;
}

void
RotateTool::down(const Vector2i& pos)
{
  Vector2f mouse_pos = viewer->get_state().screen2world(pos);

  rotate_active    = true;
  selection_center = viewer->get_workspace()->get_selection().get_center();
  
  start_angle = atan2f(selection_center.y - mouse_pos.y,
                       selection_center.x - mouse_pos.x);
}

/* EOF */
