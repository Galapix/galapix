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

#include <iostream>
#include "../math/vector2i.hpp"
#include "../math/rect.hpp"
#include "../jpeg.hpp"
#include "../file_entry.hpp"
#include "../tile_entry.hpp"
#include "tile_generation_job.hpp"

TileGenerationJob::TileGenerationJob(const FileEntry& entry, int min_scale, int max_scale,
                                     const boost::function<void (TileEntry)>& callback)
  : entry(entry),
    min_scale(min_scale),
    max_scale(max_scale),
    callback(callback)
{ 
}

void
TileGenerationJob::run()
{
  SoftwareSurface::FileFormat format = SoftwareSurface::get_fileformat(entry.get_url());

  // FIXME: Need to check for files in archives too
  if (format != SoftwareSurface::JPEG_FILEFORMAT)
    { // Generate all tiles instead of just the requested for non-jpeg formats
      min_scale = 0;
      max_scale = entry.get_thumbnail_scale();
    }

  std::cout << "TileGeneratorThread: processing scales " << min_scale << "-" << max_scale << ": " << entry.get_url() << std::endl;

  // Find scale at which the image fits on one tile
  int width  = entry.get_width();
  int height = entry.get_height();
  int scale  = min_scale;

  SoftwareSurface surface;

  switch(format)
    {
      case SoftwareSurface::JPEG_FILEFORMAT:
        {
          // The JPEG class can only scale down by factor 2,4,8, so we have to
          // limit things (FIXME: is that true? if so, why?)
          int jpeg_scale = Math::min(Math::pow2(scale), 8);
              
          if (entry.get_url().has_stdio_name())
            {
              surface = JPEG::load_from_file(entry.get_url().get_stdio_name(), jpeg_scale);
            }
          else
            {
              Blob blob = entry.get_url().get_blob();
              surface = JPEG::load_from_mem(blob.get_data(), blob.size(), jpeg_scale);
            }
      
          surface = surface.scale(Size(width  / Math::pow2(scale),
                                       height / Math::pow2(scale)));
        }
        break;

      default:
        {
          // FIXME: This is terrible, min/max_scale are meaningless
          // for non-jpeg formats, so we should just forget them
          surface = SoftwareSurface::from_url(entry.get_url());
          surface = surface.scale(Size(width  / Math::pow2(scale),
                                       height / Math::pow2(scale)));
        }
        break;
    }

  do
    {
      if (scale != min_scale)
        {
          surface = surface.halve();
        }

      for(int y = 0; 256*y < surface.get_height(); ++y)
        for(int x = 0; 256*x < surface.get_width(); ++x)
          {
            SoftwareSurface croped_surface = surface.crop(Rect(Vector2i(x * 256, y * 256),
                                                               Size(256, 256)));

            // FIXME: This is slow, should group all tiles into a single sqlite transaction
            callback(TileEntry(entry.get_fileid(), scale, Vector2i(x, y), croped_surface));
          }

      scale += 1;

    } while (scale <= max_scale);

  std::cout << "TileGeneratorThread: processing scales " << min_scale << "-" << max_scale << ": " << entry.get_url() << ": done" << std::endl;
}

/* EOF */
