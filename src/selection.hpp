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

#ifndef HEADER_SELECTION_HPP
#define HEADER_SELECTION_HPP

#include "math/vector2f.hpp"
#include "image.hpp"

class SelectionImpl;

class Selection
{
public:
  typedef std::vector<Image> Images;
  typedef std::vector<Image>::iterator iterator;

  Selection();

  void   add_image(const Image& image);
  void   add_images(const std::vector<Image>& images);
  void   remove_image(const Image& image);
  void   clear();
  Images get_images() const;
  bool   empty() const;
  bool   has(const Image& image) const;

  void scale(float factor);

  Vector2f get_center() const;

  iterator begin();
  iterator end();
  
private:
  boost::shared_ptr<SelectionImpl> impl;
};

#endif

/* EOF */
