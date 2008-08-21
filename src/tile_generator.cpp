/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
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
    } while (surface.get_width() > 32 ||
             surface.get_height() > 32);
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
  int width  = entry.size.width;
  int height = entry.size.height;
  int scale  = 0;
  while (width  / Math::pow2(scale) > 256 || 
         height / Math::pow2(scale) > 256)
    scale += 1;

  //std::cout << "Loading with scale: 2^" << scale << " = " << Math::pow2(scale) << " " << entry.size << std::endl;

  int jpeg_scale = Math::min(8, Math::pow2(scale));

  // Load the largest scale at which the image fits on a single tile 
  SoftwareSurface surface = JPEG::load_from_file(entry.filename, jpeg_scale);

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
      tile.fileid  = entry.fileid;
      tile.scale   = scale;
      tile.pos.x   = 0;
      tile.pos.y   = 0;
      tile.surface = surface;
          
      callback(tile);

      // FIXME: Might barf if width/height get == 0
      if (surface.get_width()  < 32 &&
          surface.get_height() < 32)
        {
          break;
        }

      surface = surface.halve();
      scale += 1;
    }
}

/* EOF */
