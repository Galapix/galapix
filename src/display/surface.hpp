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

#ifndef HEADER_GALAPIX_DISPLAY_SURFACE_HPP
#define HEADER_GALAPIX_DISPLAY_SURFACE_HPP

#include <memory>

#include "display/texture.hpp"
#include "math/vector2f.hpp"

class SurfaceImpl;
class Rect;
class Rectf;
class Surface;

typedef std::shared_ptr<Surface> SurfacePtr;

class Surface
{
private:
  explicit Surface(const SoftwareSurfacePtr& src, const Rect& srcrect);
  explicit Surface(const SoftwareSurfacePtr& src);

public:
  void draw(const Vector2f& pos);
  void draw(const Rectf& dstrect);
  void draw(const Rectf& srcrect, const Rectf& dstrect);

  int  get_width()  const;
  int  get_height() const;
  Size get_size() const;

  static SurfacePtr create(const SoftwareSurfacePtr& src, const Rect& srcrect);
  static SurfacePtr create(const SoftwareSurfacePtr& src);

private:
  std::unique_ptr<SurfaceImpl> impl;
};

#endif

/* EOF */
