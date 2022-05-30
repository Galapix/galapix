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

#include "tools/pan_tool.hpp"

#include "galapix/viewer.hpp"

PanTool::PanTool(Viewer* viewer_)
  : Tool(viewer_),
    trackball_mode(false),
    move_active(false),
    mouse_pos()
{
}

PanTool::~PanTool()
{
}

void
PanTool::move(Vector2i const& pos, Vector2i const& rel)
{
  mouse_pos = pos;

  if (trackball_mode)
  {
    // Movement speed should be configurable
    viewer->get_state().move(glm::vec2(rel.as_vec()) * 4.0f);
  }
  else if (move_active)
  {
    // FIXME: This is of course wrong, since depending on x/yrel will lead to drift
    viewer->get_state().move(glm::vec2(rel.as_vec()) * 4.0f);
  }
}

void
PanTool::up(Vector2i const& pos)
{
  mouse_pos   = pos;
  move_active = false;
}

void
PanTool::down(Vector2i const& pos)
{
  mouse_pos   = pos;
  move_active = true;
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
