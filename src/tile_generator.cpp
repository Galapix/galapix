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
#include "math/size.hpp"
#include "math/rect.hpp"
#include "jpeg.hpp"
#include "file_entry.hpp"
#include "tile_entry.hpp"
#include "tile_generator.hpp"

TileGenerator::TileGenerator()
{
}

TileGenerator::~TileGenerator()
{
}

void
TileGenerator::generate_all(int fileid, const SoftwareSurface& surface_, 
                            const boost::function<void (TileEntry)>& callback)
{
  int scale = 0;

  SoftwareSurface surface = surface_;

  do
    {
      if (scale != 0)
        {
          surface = surface.scale(Size(surface.get_width()/2, 
                                       surface.get_height()/2));
        }

      for(int y = 0; 256*y < surface.get_height(); ++y)
        for(int x = 0; 256*x < surface.get_width(); ++x)
          {
            SoftwareSurface croped_surface = surface.crop(Rect(Vector2i(x * 256, y * 256),
                                                               Size(256, 256)));

            TileEntry tile;
            tile.fileid = fileid;
            tile.scale  = scale;
            tile.pos.x = x;
            tile.pos.y = y;
            tile.surface = croped_surface;
          
            callback(tile);
          }

      scale += 1;

      //std::cout << "Scale: " << scale << " - " << surface.get_size() << std::endl;
    } 
  while (surface.get_width()/2  != 0 &&
         surface.get_height()/2 != 0);
}

void
TileGenerator::generate_all(int fileid, const std::string& filename,
                            const boost::function<void (TileEntry)>& callback)
{
  generate_all(fileid, SoftwareSurface::from_file(filename), callback);
}

void
TileGenerator::generate_quick(const FileEntry& entry,
                              const boost::function<void (TileEntry)>& callback)
{
  // Find scale at which the image fits on one tile
  int width  = entry.get_width();
  int height = entry.get_height();
  int scale  = 0;
  while (width  / Math::pow2(scale) > 256 || 
         height / Math::pow2(scale) > 256)
    scale += 1;

  //std::cout << "Loading with scale: 2^" << scale << " = " << Math::pow2(scale) << " " << entry.size << std::endl;

  // The JPEG class can only scale down by factor 2,4,8, so we have to limit things
  int jpeg_scale = Math::min(8, Math::pow2(scale));

  // Load the largest scale at which the image fits on a single tile 
  SoftwareSurface surface = JPEG::load_from_file(entry.get_filename(), jpeg_scale);

  // The result of JPEG::load_from_file might be larger then the requested size, so scale it down
  // FIXME: We should not throw this data away, now that we already
  // have loaded it! Instead we should crop it and place it in the
  // tile cache
  if (surface.get_width()  > 256 ||
      surface.get_height() > 256)
    {
      surface = surface.scale(Size(width  / Math::pow2(scale),
                                   height / Math::pow2(scale)));
    }

  //std::cout << " => " << surface.get_size() << std::endl;

  while (true)
    {
      //std::cout << scale << " size: " << surface.get_size() << std::endl;

      TileEntry tile;
      tile.fileid  = entry.get_fileid();
      tile.scale   = scale;
      tile.pos.x   = 0;
      tile.pos.y   = 0;
      tile.surface = surface;
          
      callback(tile);

      if (surface.get_width()/2  == 0 ||
          surface.get_height()/2 == 0)
        {
          break;
        }

      surface = surface.halve();
      scale += 1;
    }
}

/* EOF */
