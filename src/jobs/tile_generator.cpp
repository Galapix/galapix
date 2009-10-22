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
#include "database/tile_entry.hpp"
#include "math/rect.hpp"
#include "math/vector2i.hpp"
#include "plugins/jpeg.hpp"
#include "util/software_surface.hpp"

void
TileGenerator::generate(const FileEntry& m_file_entry, 
                        SoftwareSurfaceFactory::FileFormat format,
                        int m_min_scale_in_db, int m_max_scale_in_db,
                        int min_scale, int max_scale,
                        const boost::function<void(const TileEntry& tile_entry)>& callback)
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
  
  // Find scale at which the image fits on one tile
  int width  = m_file_entry.get_width();
  int height = m_file_entry.get_height();
  int scale  = min_scale;

  SoftwareSurfaceHandle surface;

  switch(format)
  {
    case SoftwareSurfaceFactory::JPEG_FILEFORMAT:
    {
      // The JPEG class can only scale down by factor 2,4,8, so we have to
      // limit things (FIXME: is that true? if so, why?)
      int jpeg_scale = Math::min(Math::pow2(scale), 8);
              
      if (m_file_entry.get_url().has_stdio_name())
      {
        surface = JPEG::load_from_file(m_file_entry.get_url().get_stdio_name(), jpeg_scale);
      }
      else
      {
        BlobHandle blob = m_file_entry.get_url().get_blob();
        surface = JPEG::load_from_mem(blob->get_data(), blob->size(), jpeg_scale);
      }
      
      surface = surface->scale(Size(width  / Math::pow2(scale),
                                    height / Math::pow2(scale)));
    }
    break;

    default:
    {
      // FIXME: This is terrible, min/max_scale are meaningless
      // for non-jpeg formats, so we should just forget them
      surface = SoftwareSurfaceFactory::from_url(m_file_entry.get_url());
      surface = surface->scale(Size(width  / Math::pow2(scale),
                                    height / Math::pow2(scale)));
    }
    break;
  }

  do
  {
    if (scale != min_scale)
    {
      surface = surface->halve();
    }

    for(int y = 0; 256*y < surface->get_height(); ++y)
      for(int x = 0; 256*x < surface->get_width(); ++x)
      {
        SoftwareSurfaceHandle croped_surface = surface->crop(Rect(Vector2i(x * 256, y * 256),
                                                                  Size(256, 256)));

        callback(TileEntry(m_file_entry, scale, Vector2i(x, y), croped_surface));
      }

    scale += 1;
  } 
  while (scale <= max_scale);

  if (0)
    std::cout << "TileGeneratorThread: processing scales "
              << min_scale << "-" << max_scale << ": " << m_file_entry.get_url() << ": done" << std::endl;
}

/* EOF */
