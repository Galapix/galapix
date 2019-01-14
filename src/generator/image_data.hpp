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

#ifndef HEADER_GALAPIX_GENERATOR_IMAGE_DATA_HPP
#define HEADER_GALAPIX_GENERATOR_IMAGE_DATA_HPP

#include "resource/image_info.hpp"
#include "galapix/tile.hpp"

class ImageData
{
public:
  ImageData() :
    m_image_info(),
    m_image_tiles()
  {}

  ImageData(const ImageInfo& image_info,
            std::vector<Tile> image_tiles) :
    m_image_info(image_info),
    m_image_tiles(std::move(image_tiles))
  {}

  ImageInfo get_image_info() const { return m_image_info; }
  const std::vector<Tile>& get_image_tiles() const { return m_image_tiles; }

private:
  // ResourceLocator m_locator;
  // ResoruceInfo m_resource_info
  ImageInfo m_image_info;
  std::vector<Tile> m_image_tiles;

private:
  ImageData(const ImageData&) = delete;
  ImageData& operator=(const ImageData&) = delete;
};

#endif

/* EOF */
