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

#ifndef HEADER_GALAPIX_DISPLAY_TEXTURE_HPP
#define HEADER_GALAPIX_DISPLAY_TEXTURE_HPP

#include <boost/scoped_ptr.hpp>
#include <memory>

#include "util/software_surface.hpp"

class Rect;
class Size;
class SoftwareSurface;
class TextureImpl;
class Texture;

typedef std::shared_ptr<Texture> TexturePtr;

class Texture
{
private:
  Texture(const SoftwareSurfacePtr& src, const Rect& srcrect);

public:
  static TexturePtr create(const SoftwareSurfacePtr& src, const Rect& srcrect);

  int get_width() const;
  int get_height() const;
  
  void bind();

private:
  boost::scoped_ptr<TextureImpl> impl;
};

#endif

/* EOF */
