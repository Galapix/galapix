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

#ifndef HEADER_TOOL_HPP
#define HEADER_TOOL_HPP

#include "math/vector2f.hpp"

class Viewer;

class Tool
{
protected:
  Viewer* viewer;

public:
  Tool(Viewer* viewer) : viewer(viewer) {}
  virtual ~Tool() {}

  virtual void mouse_move(const Vector2f& pos, const Vector2f& rel) =0;
  virtual void mouse_btn_up  (int num, const Vector2f& pos) =0;
  virtual void mouse_btn_down(int num, const Vector2f& pos) =0;

  virtual void draw() =0;

private:
  Tool (const Tool&);
  Tool& operator= (const Tool&);
};

#endif

/* EOF */
