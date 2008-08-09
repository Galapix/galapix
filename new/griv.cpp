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

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include "FreeImage.h"

#include "surface.hpp"
#include "framebuffer.hpp"
#include "math/size.hpp"
#include "math/rect.hpp"
#include "math/vector2i.hpp"
#include "url.hpp"
#include "sqlite.hpp"
#include "software_surface.hpp"
#include "file_database.hpp"
#include "tile_database.hpp"
#include "filesystem.hpp"
#include "workspace.hpp"
#include "viewer.hpp"
#include "griv.hpp"

Griv::Griv()
{
  FreeImage_Initialise();
  Filesystem::init();
}

Griv::~Griv()
{
  Filesystem::deinit();
  FreeImage_DeInitialise();
}

void
Griv::generate_tiles(const std::vector<std::string>& filenames)
{
  SQLiteConnection db("test.sqlite");

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  for(std::vector<std::string>::size_type i = 0; i < filenames.size(); ++i)
    {
      FileEntry entry;
      std::cout << "Getting file entry..." << std::endl;
      if (!file_db.get_file_entry(filenames[i], entry))
        {
          std::cout << "Couldn't find entry for " << filenames[i] << std::endl;
        }
      else
        {
          // Generate Image Tiles
          std::cout << "Generating tiles... " << filenames[i]  << std::endl;
          SoftwareSurface surface(filenames[i]);
          std::cout << "Image loading" << std::endl;      

          int scale = 0;

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

                    Tile tile;
                    tile.fileid = entry.fileid;
                    tile.scale  = scale;
                    tile.x = x;
                    tile.y = y;
                    tile.surface = croped_surface;
          
                    tile_db.store_tile(tile);
                  }

              scale += 1;

              std::cout << "Scale: " << scale << " - " << surface.get_size() << std::endl;
            } while (surface.get_width() > 32 ||
                     surface.get_height() > 32);
        }
    }
}

void
Griv::view(const std::vector<std::string>& filenames)
{
  SQLiteConnection db("test.sqlite");

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  Workspace workspace;

  for(std::vector<std::string>::size_type i = 0; i < filenames.size(); ++i)
    {
      FileEntry entry;
      //std::cout << "Getting file entry..." << std::endl;
      if (!file_db.get_file_entry(filenames[i], entry))
        {
          std::cout << "Couldn't find entry for " << filenames[i] << std::endl;
        }
      else
        {
          workspace.add_image(entry.fileid, entry.filename, entry.size);
        }

      std::cout << "Adding images to workspace " << i+1 << "/" << filenames.size() << "\r" << std::flush;
    }
  std::cout << std::endl;

  Framebuffer::init();

  workspace.layout(4,3);

  Viewer viewer;

  Uint32 ticks = SDL_GetTicks();
  while(!viewer.done())
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
        viewer.process_event(event);

      Uint32 cticks = SDL_GetTicks();
      float delta = (cticks - ticks) / 1000.0f;
      ticks = cticks;

      viewer.update(delta);

      if (1) // if something has changed, redraw
        {
      Framebuffer::clear();
      viewer.draw(workspace);
      Framebuffer::flip();
        }

      SDL_Delay(30);
    }

  Framebuffer::deinit();
}

int
Griv::main(int argc, char** argv)
{
  if (argc < 2)
    {
      std::cout << "Usage: " << argv[0] << " view [FILES]...\n"
                << "       " << argv[0] << " prepare [FILES]...\n";
    }
  else
    {
      std::vector<std::string> filenames;
      for(int i = 2; i < argc; ++i)
        filenames.push_back(Filesystem::realpath(argv[i]));

      if (strcmp(argv[1], "view") == 0)
        {
          view(filenames);
        }
      else if (strcmp(argv[1], "prepare") == 0)
        {
          generate_tiles(filenames);
        }
      else
        {
          std::cout << "Usage: " << argv[0] << " view [FILES]...\n"
                    << "       " << argv[0] << " prepare [FILES]...\n";
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
  catch(const std::exception& err) 
    {
      std::cout << "Exception: " << err.what() << std::endl;
    }
}
  
/* EOF */
