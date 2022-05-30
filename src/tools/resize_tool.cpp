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

#include "tools/resize_tool.hpp"

#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"

ResizeTool::ResizeTool(Viewer* viewer_)
  : Tool(viewer_),
    resize_active(false),
    resize_center(),
    selection_center(),
    old_scale(1.0f)
{
}

void
ResizeTool::move(Vector2i const& pos, Vector2i const& /*rel*/)
{
  if (resize_active)
  {
    Vector2f p = viewer->get_state().screen2world(pos);

    float a = geom::distance(selection_center, p);
    float b = geom::distance(selection_center, resize_center);

    if (b != 0.0f)
    {
      //std::cout << a << " " << b << " " << a / b << std::endl;
      viewer->get_workspace()->get_selection()->scale((1.0f/old_scale) * (a/b));
      old_scale = a/b; // FIXME: Hack, should scale to original scale
    }
  }
}

void
ResizeTool::up(Vector2i const& /*pos*/)
{
  resize_active = false;
  old_scale = 1.0f;
}

void
ResizeTool::down(Vector2i const& pos)
{
  resize_active    = true;
  resize_center    = viewer->get_state().screen2world(pos);
  selection_center = viewer->get_workspace()->get_selection()->get_center();
}

/* EOF */
