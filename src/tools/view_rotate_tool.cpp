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

#include "tools/view_rotate_tool.hpp"

#include "display/framebuffer.hpp"
#include "galapix/viewer.hpp"

ViewRotateTool::ViewRotateTool(Viewer* viewer_)
  : Tool(viewer_),
    active(false),
    start_angle(0.0f)
{
}

void
ViewRotateTool::move(const Vector2i& pos, const Vector2i& rel)
{
  if (active)
  {
    Vector2i center(Framebuffer::get_width()/2,
                    Framebuffer::get_height()/2);

    float angle = atan2f(static_cast<float>(pos.y - center.y),
                         static_cast<float>(pos.x - center.x));

    viewer->get_state().rotate((angle - start_angle)/Math::pi * 180.0f);
    start_angle = angle;
  }
}

void
ViewRotateTool::up  (const Vector2i& /*pos*/)
{
  active = false;
}

void
ViewRotateTool::down(const Vector2i& pos)
{
  active = true;

  Vector2i center(Framebuffer::get_width()/2,
                  Framebuffer::get_height()/2);

  start_angle = atan2f(static_cast<float>(pos.y - center.y),
                       static_cast<float>(pos.x - center.x));
}

/* EOF */
