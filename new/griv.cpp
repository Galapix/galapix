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

#include <sstream>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include "FreeImage.h"

#include "math/size.hpp"
#include "math/rect.hpp"
#include "math/vector2i.hpp"
#include "url.hpp"
#include "sqlite.hpp"
#include "software_surface.hpp"
#include "file_database.hpp"
#include "tile_database.hpp"
#include "griv.hpp"

Griv::Griv()
{
  FreeImage_Initialise();
}

Griv::~Griv()
{
  FreeImage_DeInitialise();
}

int
Griv::main(int argc, char** argv)
{
  std::vector<std::string> rest;

  for(int i = 1; i < argc; ++i)
    {
      if (argv[i][0] == '-')
        { // option
          
        }
      else
        { // rest
          rest.push_back(argv[i]);
        }
    }

  SQLiteConnection db("test.sqlite");

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  for(std::vector<std::string>::size_type i = 0; i < rest.size(); ++i)
    {
      if (0) // Tile Generation
        {
          // Generate Image Tiles
          SoftwareSurface surface(rest[i]);
      
          int scale = 0;

          do
            {
              if (scale != 0)
                {
                  surface = surface.scale(Size(surface.get_width()/2, 
                                               surface.get_height()/2));
                }

              for(int y = 0; y <= surface.get_height()/256; ++y)
                for(int x = 0; x <= surface.get_width()/256; ++x)
                  {
                    std::ostringstream out;
                    out << "/tmp/out/tile-" << scale << "-" << y << "+" << x << ".jpg";

                    SoftwareSurface croped_surface = surface.crop(Rect(Vector2i(x * 256, y * 256),
                                                                       Size(256, 256)));
                    croped_surface.save(out.str());
                  }

              scale += 1;
            } while (surface.get_width() > 32 ||
                     surface.get_height() > 32);
        }
      else  // Data base test
        {
          FileEntry entry;
          if (file_db.get_file_entry(rest[i], entry))
            std::cout << entry << std::endl;
          else
            std::cout << "Couldn't find entry for " << rest[i] << std::endl;
        }
    }

  return 0;
}

int main(int argc, char** argv)
{
  try 
    {
      Griv app;
      int ret = app.main(argc, argv);
      return ret;
    }
  catch(std::exception& err) 
    {
      std::cout << "Exception: " << err.what() << std::endl;
    }
}

/* EOF */
