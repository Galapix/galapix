// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GALAPIX_SELECTION_HPP
#define HEADER_GALAPIX_GALAPIX_SELECTION_HPP

#include <memory>

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"

class Selection;

using SelectionPtr = std::shared_ptr<Selection>;

class Selection
{
public:
  using iterator = ImageCollection::iterator;
  using const_iterator = ImageCollection::const_iterator;

private:
  Selection();

public:
  static SelectionPtr create() { return SelectionPtr(new Selection); }

  void add_image(WorkspaceItemPtr const& image);
  void add_images(ImageCollection const& images);
  void remove_image(WorkspaceItemPtr const& image);
  void clear();
  ImageCollection get_images() const;
  bool empty() const;
  bool has(WorkspaceItemPtr const& image) const;

  void scale(float factor);

  Vector2f get_center() const;

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  Rectf get_bounding_rect() const;

private:
  ImageCollection m_images;
};

#endif

/* EOF */
