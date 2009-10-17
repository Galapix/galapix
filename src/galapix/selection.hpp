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

#ifndef HEADER_GALAPIX_GALAPIX_SELECTION_HPP
#define HEADER_GALAPIX_GALAPIX_SELECTION_HPP

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"

class SelectionImpl;

class Selection
{
public:
  typedef ImageCollection::iterator iterator;
  typedef ImageCollection::const_iterator const_iterator;

  Selection();

  void   add_image(const ImageHandle& image);
  void   add_images(const ImageCollection& images);
  void   remove_image(const ImageHandle& image);
  void   clear();
  ImageCollection get_images() const;
  bool   empty() const;
  bool   has(const ImageHandle& image) const;

  void scale(float factor);

  Vector2f get_center() const;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end()   const;

  Rectf get_bounding_rect() const;
  
private:
  boost::shared_ptr<SelectionImpl> impl;
};

#endif

/* EOF */
