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

#include "galapix/image_collection.hpp"

#include <algorithm>

ImageCollection::ImageCollection() :
  m_images()
{
}

ImageCollection::~ImageCollection()
{
}

void
ImageCollection::add(WorkspaceItemPtr const& image)
{
  m_images.push_back(image);
}

void
ImageCollection::remove(WorkspaceItemPtr const& image)
{
  m_images.erase(std::remove(m_images.begin(), m_images.end(), image),
                 m_images.end());
}

ImageCollection::iterator
ImageCollection::begin()
{
  return m_images.begin();
}

ImageCollection::iterator
ImageCollection::end()
{
  return m_images.end();
}

ImageCollection::const_iterator
ImageCollection::begin() const
{
  return m_images.begin();
}

ImageCollection::const_iterator
ImageCollection::end() const
{
  return m_images.end();
}

ImageCollection::reverse_iterator
ImageCollection::rbegin()
{
  return m_images.rbegin();
}

ImageCollection::reverse_iterator
ImageCollection::rend()
{
  return m_images.rend();
}

ImageCollection::const_reverse_iterator
ImageCollection::rbegin() const
{
  return m_images.rbegin();
}

ImageCollection::const_reverse_iterator
ImageCollection::rend() const
{
  return m_images.rend();
}

ImageCollection::size_type
ImageCollection::size() const
{
  return m_images.size();
}

bool
ImageCollection::empty() const
{
  return m_images.empty();
}

/* EOF */
