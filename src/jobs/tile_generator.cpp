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

#include "jobs/tile_generator.hpp"

#include <iostream>

#include "database/file_entry.hpp"
#include "galapix/tile.hpp"
#include "math/rect.hpp"
#include "math/vector2i.hpp"
#include "plugins/jpeg.hpp"
#include "util/software_surface.hpp"

void
TileGenerator::generate(const FileEntry& m_file_entry, 
                        int m_min_scale_in_db, int m_max_scale_in_db,
                        int min_scale, int max_scale,
                        const boost::function<void(Tile)>& callback)
{
  if (true /* verbose */)
  {
    std::cout << "TileGenerator::generate(): have ";
    if (m_min_scale_in_db == -1 && m_max_scale_in_db == -1)
    {
      std::cout << "[empty]";
    }
    else
    {
      std::cout << "[" << m_min_scale_in_db << ".." << m_max_scale_in_db << "]";
    }
    std::cout << " generating ["
              << min_scale << ".." << max_scale << "]: " << m_file_entry.get_fileid()
              << ": " 
              << m_file_entry.get_url() << std::endl;
  }

  generate(m_file_entry.get_url(), m_file_entry.get_image_size(), min_scale, max_scale, callback);

  if (0)
    std::cout << "TileGeneratorThread: processing scales "
              << min_scale << "-" << max_scale << ": " << m_file_entry.get_url() << ": done" << std::endl;
}

void
TileGenerator::generate(const URL& url, const Size& orignal_size,
                        int min_scale, int max_scale,
                        const boost::function<void(Tile)>& callback)
{
  // Load the image, try to load an already downsized version if possible
  SoftwareSurfacePtr surface = load_surface(url, min_scale);

  // Scale the image if loading a downsized version was not possible
  Size target_size(orignal_size.width  / Math::pow2(min_scale),
                   orignal_size.height / Math::pow2(min_scale));

  if (target_size != surface->get_size())
  {
    surface = surface->scale(target_size);
  }

  cut_into_tiles(surface, min_scale, max_scale, callback);
}

SoftwareSurfacePtr
TileGenerator::load_surface(const URL& url, int min_scale)
{
  // Load the image
  switch(SoftwareSurfaceFactory::get_fileformat(url))
  {
    case SoftwareSurfaceFactory::JPEG_FILEFORMAT:
    {
      // The JPEG class can only scale down by factor 2,4,8, so we have to
      // limit things (FIXME: is that true? if so, why?)
      int jpeg_scale = Math::min(Math::pow2(min_scale), 8);
              
      if (url.has_stdio_name())
      {
        return JPEG::load_from_file(url.get_stdio_name(), jpeg_scale);
      }
      else
      {
        BlobPtr blob = url.get_blob();
        return JPEG::load_from_mem(blob->get_data(), blob->size(), jpeg_scale);
      }
    }
    break;

    default:
      return SoftwareSurfaceFactory::from_url(url);
      break;
  }
}

void
TileGenerator::cut_into_tiles(SoftwareSurfacePtr surface,
                              int min_scale, int max_scale,
                              const boost::function<void (Tile)>& callback)
{
  // Cut the given image into tiles, give created tiles to callback(),
  // surface is expected to be pre-scaled and already at min_scale size
  int scale = min_scale;
  do
  {
    if (scale != min_scale)
    {
      surface = surface->halve();
    }

    for(int y = 0; 256*y < surface->get_height(); ++y)
      for(int x = 0; 256*x < surface->get_width(); ++x)
      {
        SoftwareSurfacePtr croped_surface = surface->crop(Rect(Vector2i(x * 256, y * 256),
                                                               Size(256, 256)));

        callback(Tile(scale, Vector2i(x, y), croped_surface));
      }

    scale += 1;
  }
  while (scale <= max_scale);
}

/* EOF */
