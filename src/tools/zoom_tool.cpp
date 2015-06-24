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

#include "tools/zoom_tool.hpp"

#include "galapix/viewer.hpp"

ZoomTool::ZoomTool(Viewer* viewer_, float zoom_factor_)
  : Tool(viewer_),
    zoom_active(false),
    zoom_factor(zoom_factor_)
{
}

void
ZoomTool::move(const Vector2i& /*pos*/, const Vector2i& /*rel*/)
{
}

void
ZoomTool::up  (const Vector2i& /*pos*/)
{
  zoom_active = false;
}

void
ZoomTool::down(const Vector2i& /*pos*/)
{
  zoom_active = true;
}

void
ZoomTool::update(const Vector2i& mouse_pos, float delta)
{
  if (zoom_active)
  {
    if (viewer->get_trackball_mode())
    {
      if (zoom_factor > 0)
      {
        viewer->get_state().zoom(1.0f / (1.0f + zoom_factor * delta));
      }
      else
      {
        viewer->get_state().zoom(1.0f - zoom_factor * delta);
      }
    }
    else
    {
      if (zoom_factor > 0)
      {
        viewer->get_state().zoom(1.0f / (1.0f + zoom_factor * delta), mouse_pos);
      }
      else
      {
        viewer->get_state().zoom(1.0f - zoom_factor * delta, mouse_pos);
      }
    }
  }
}

void
ZoomTool::draw()
{
}

bool
ZoomTool::is_active() const
{
  return zoom_active;
}

/* EOF */
