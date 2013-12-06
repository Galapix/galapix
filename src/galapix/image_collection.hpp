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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_COLLECTION_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_COLLECTION_HPP

#include <vector>

#include "galapix/image_handle.hpp"

/** A wrapper around a std::vector<> */
class ImageCollection
{
public:
  typedef std::vector<ImagePtr> Images;
  typedef Images::iterator iterator;
  typedef Images::const_iterator const_iterator;
  typedef Images::reverse_iterator reverse_iterator;
  typedef Images::const_reverse_iterator const_reverse_iterator;
  typedef Images::size_type  size_type;
  
private:
  Images m_images;

public:
  ImageCollection();
  ~ImageCollection();

  void add(ImagePtr image);
  void remove(ImagePtr image);
  void clear() { m_images.clear(); }

  iterator begin();
  iterator end();

  const_iterator begin() const;
  const_iterator end() const;

  reverse_iterator rbegin();
  reverse_iterator rend();

  const_reverse_iterator rbegin() const;
  const_reverse_iterator rend() const;

  ImagePtr& front() { return m_images.front(); }
  const ImagePtr& front() const { return m_images.front(); }

  ImagePtr& back() { return m_images.back(); }
  const ImagePtr& back() const { return m_images.back(); }

  size_type size() const;
  bool empty() const;

  void erase(iterator first, iterator last) { m_images.erase(first, last); }

  ImagePtr& operator[](size_type i) { return m_images[i]; }
  const ImagePtr& operator[](size_type i) const { return m_images[i]; }
};

#endif

/* EOF */
