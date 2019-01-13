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

#ifndef HEADER_GALAPIX_GALAPIX_TOOL_HPP
#define HEADER_GALAPIX_GALAPIX_TOOL_HPP

#include "math/vector2f.hpp"

class Viewer;

class Tool
{
public:
  Tool(Viewer* viewer_) : viewer(viewer_) {}
  virtual ~Tool() {}

  virtual void move(const Vector2i& pos, const Vector2i& rel) =0;
  virtual void up  (const Vector2i& pos) =0;
  virtual void down(const Vector2i& pos) =0;

  virtual void draw() =0;

protected:
  Viewer* viewer;

private:
  Tool (const Tool&);
  Tool& operator= (const Tool&);
};

#endif

/* EOF */
