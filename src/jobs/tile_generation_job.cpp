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

#include "jobs/tile_generation_job.hpp"

#include <iostream>

#include "math/rect.hpp"
#include "plugins/jpeg.hpp"
#include "util/software_surface_factory.hpp"
#include "database/tile_entry.hpp"

TileGenerationJob::TileGenerationJob(const JobHandle& job_handle, const FileEntry& entry_, int min_scale_, int max_scale_,
                                     const boost::function<void (TileEntry)>& callback_)
  : Job(job_handle),
    entry(entry_),
    min_scale(min_scale_),
    max_scale(max_scale_),
    callback(callback_)
{ 
}

void
TileGenerationJob::run()
{
  SoftwareSurfaceFactory::FileFormat format = SoftwareSurfaceFactory::get_fileformat(entry.get_url());

  // FIXME: Need to check for files in archives too
  if (format != SoftwareSurfaceFactory::JPEG_FILEFORMAT)
    { // Generate all tiles instead of just the requested for non-jpeg formats
      min_scale = 0;
      max_scale = entry.get_thumbnail_scale();
    }

  std::cout << "TileGeneratorThread: processing scales " << min_scale << "-" << max_scale << ": " << entry.get_url() << std::endl;

  // Find scale at which the image fits on one tile
  int width  = entry.get_width();
  int height = entry.get_height();
  int scale  = min_scale;

  SoftwareSurfaceHandle surface;

  switch(format)
    {
      case SoftwareSurfaceFactory::JPEG_FILEFORMAT:
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
              BlobHandle blob = entry.get_url().get_blob();
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
          surface = SoftwareSurfaceFactory::from_url(entry.get_url());
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

            // FIXME: This is slow, should group all tiles into a single sqlite transaction
            if (callback)
              callback(TileEntry(entry, scale, Vector2i(x, y), croped_surface));
          }

      scale += 1;

    } 
  while (scale <= max_scale);

  std::cout << "TileGeneratorThread: processing scales "
            << min_scale << "-" << max_scale << ": " << entry.get_url() << ": done" << std::endl;
}

/* EOF */
