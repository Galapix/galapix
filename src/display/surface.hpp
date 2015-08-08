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

class Surface
{
public:
  static Surface create(SoftwareSurface const& src, Rect const& srcrect);
  static Surface create(SoftwareSurface const& src);

private:
  explicit Surface(SoftwareSurface const& src, Rect const& srcrect);
  explicit Surface(SoftwareSurface const& src);

public:
  Surface();

  void draw(const Vector2f& pos) const;
  void draw(const Rectf& dstrect) const;
  void draw(const Rectf& srcrect, const Rectf& dstrect) const;

  int  get_width()  const;
  int  get_height() const;
  Size get_size() const;

  explicit operator bool() const { return m_impl != nullptr; }

private:
  std::shared_ptr<const SurfaceImpl> m_impl;
};

#endif

/* EOF */
