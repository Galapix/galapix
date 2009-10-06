/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include "tools/resize_tool.hpp"

#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"

ResizeTool::ResizeTool(Viewer* viewer_)
  : Tool(viewer_),
    resize_active(false),
    old_scale(1.0f)
{  
}

void
ResizeTool::move(const Vector2i& pos, const Vector2i& /*rel*/)
{
  if (resize_active)
    {
      Vector2f p = viewer->get_state().screen2world(pos);

      float a = (selection_center - p).length();
      float b = (selection_center - resize_center).length();

      if (b != 0.0f)
        {
          //std::cout << a << " " << b << " " << a / b << std::endl;
          viewer->get_workspace()->get_selection().scale((1.0f/old_scale) * (a/b));
          old_scale = a/b; // FIXME: Hack, should scale to original scale 
        }
    }
}

void
ResizeTool::up(const Vector2i& /*pos*/)
{
  resize_active = false;
  old_scale = 1.0f;
}

void
ResizeTool::down(const Vector2i& pos)
{
  resize_active    = true;
  resize_center    = viewer->get_state().screen2world(pos);
  selection_center = viewer->get_workspace()->get_selection().get_center();
}

/* EOF */
