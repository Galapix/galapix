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

#ifndef HEADER_PAN_TOOL_HPP
#define HEADER_PAN_TOOL_HPP

#include "tool.hpp"

class Viewer;

class PanTool : public Tool
{
private:
public:
  PanTool(Viewer* viewer);
  ~PanTool();

  void mouse_move(const Vector2f& pos, const Vector2f& rel);
  void mouse_btn_up  (int num, const Vector2f& pos);
  void mouse_btn_down(int num, const Vector2f& pos);

  void draw();
};

#endif

/* EOF */
