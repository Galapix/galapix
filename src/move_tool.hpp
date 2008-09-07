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

#ifndef HEADER_MOVE_TOOL_HPP
#define HEADER_MOVE_TOOL_HPP

#include <vector>
#include "image.hpp"
#include "tool.hpp"

class MoveTool : public Tool
{
private:
  Vector2i mouse_pos;
  bool drag_active;
  bool move_active;
  Vector2f click_pos;

public:
  MoveTool(Viewer* viewer);
  ~MoveTool();

  void mouse_move(const Vector2i& pos, const Vector2i& rel);
  void mouse_btn_up  (int num, const Vector2i& pos);
  void mouse_btn_down(int num, const Vector2i& pos);

  void draw();
  void update(float delta);

private:
  MoveTool (const MoveTool&);
  MoveTool& operator= (const MoveTool&);
};

#endif

/* EOF */
