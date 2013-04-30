/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GENERATOR_IMAGE_DATA_HPP
#define HEADER_GALAPIX_GENERATOR_IMAGE_DATA_HPP

#include "resource/image_info.hpp"

class ImageData
{
private:
  // ResourceLocator m_locator;
  // ResoruceInfo m_resource_info
  ImageInfo m_image_info;
  std::vector<TileEntry> m_image_tiles;

public:
  ImageData() :
    m_info(),
    m_tiles()
  {}

  ImageInfo get_image_info() const { return m_image_info; }
  const std::vector<TileEntry>& get_image_tiles() const { return m_image_tiles; }

private:
  ImageData(const ImageData&) = delete;
  ImageData& operator=(const ImageData&) = delete;
};

#endif

/* EOF */
