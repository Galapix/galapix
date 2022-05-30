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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_COLLECTION_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_COLLECTION_HPP

#include <vector>

#include "galapix/workspace_item.hpp"

/** A wrapper around a std::vector<> */
class ImageCollection
{
public:
  using Images = std::vector<WorkspaceItemPtr>;
  using iterator = Images::iterator;
  using const_iterator = Images::const_iterator;
  using reverse_iterator = Images::reverse_iterator;
  using const_reverse_iterator = Images::const_reverse_iterator;
  using size_type = Images::size_type;

public:
  ImageCollection();
  ~ImageCollection();

  void add(WorkspaceItemPtr const& image);
  void remove(WorkspaceItemPtr const& image);

  void clear() { m_images.clear(); }

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  reverse_iterator rbegin();
  reverse_iterator rend();

  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;

  WorkspaceItemPtr& front() { return m_images.front(); }
  WorkspaceItemPtr const& front() const { return m_images.front(); }

  WorkspaceItemPtr& back() { return m_images.back(); }
  WorkspaceItemPtr const& back() const { return m_images.back(); }

  size_type size() const;
  bool empty() const;

  void erase(iterator first, iterator last) { m_images.erase(first, last); }

  WorkspaceItemPtr& operator[](size_type i) { return m_images[i]; }
  WorkspaceItemPtr const& operator[](size_type i) const { return m_images[i]; }

private:
  Images m_images;
};

#endif

/* EOF */
