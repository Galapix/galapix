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

#include <boost/bind.hpp>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>
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
#include "database_thread.hpp"
#include "filesystem.hpp"
#include "tile_generator.hpp"
#include "workspace.hpp"
#include "viewer_thread.hpp"
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
Griv::generate_tiles(const std::string& database, const std::vector<std::string>& filenames)
{
  SQLiteConnection db(database);

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  TileGenerator tile_generator;

  for(std::vector<std::string>::size_type i = 0; i < filenames.size(); ++i)
    {
      FileEntry entry;
      std::cout << "Getting file entry..." << std::endl;
      if (!file_db.get_file_entry(filenames[i], &entry))
        {
          std::cout << "Couldn't find entry for " << filenames[i] << std::endl;
        }
      else
        {
          // Generate Image Tiles
          std::cout << "Generating tiles... " << filenames[i]  << std::endl;
          SoftwareSurface surface(filenames[i]);
          std::cout << "Image loading" << std::endl;      
          
          tile_generator.generate(entry.fileid, surface, tile_db);
        }
    }
}

void
Griv::view(const std::string& database, const std::vector<std::string>& filenames)
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
      std::cout << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
      exit(1);
    }
  atexit(SDL_Quit); 

  DatabaseThread database_thread(database);
  ViewerThread viewer_thread;

  database_thread.start();
  viewer_thread.start();

  for(std::vector<std::string>::size_type i = 0; i < filenames.size(); ++i)
    database_thread.request_file(filenames[i], boost::bind(&ViewerThread::receive_file, &viewer_thread, _1));

  viewer_thread.join();
  database_thread.stop();
  database_thread.join();
}

void
Griv::print_usage()
{
      std::cout << "Usage: griv view    [OPTIONS]... [FILES]...\n"
                << "       griv prepare [OPTIONS]... [FILES]...\n"
                << "\n"
                << "Options:\n"
                << "  -d, --database FILE    Use FILE has database (default: test.sqlite)\n"
                << std::endl;
}

int
Griv::main(int argc, char** argv)
{
  // FIXME: Function doesn't seem to be available in 3.4.2
  // if (!sqlite3_threadsafe())
  //  throw std::runtime_error("Error: SQLite must be compiled with SQLITE_THREADSAFE");

  std::string database = "test.sqlite";
  
  if (argc < 2)
    {
      print_usage();
    }
  else
    {
      std::vector<std::string> filenames;
      for(int i = 2; i < argc; ++i)
        {
          if (argv[i][0] == '-')
            {
              if (strcmp(argv[i], "--help") == 0 ||
                  strcmp(argv[i], "-h") == 0)
                {
                  print_usage();
                  exit(0);
                }
              else if (strcmp(argv[i], "--database") == 0 ||
                  strcmp(argv[i], "-d") == 0)
                {
                  ++i;
                  if (i < argc)
                    {
                      database = argv[i];
                    }
                  else
                    {
                      throw std::runtime_error(std::string(argv[i-1]) + " requires an argument");
                    }
                }
              else
                {
                  throw std::runtime_error("Unknown option " + std::string(argv[i]));
                }
            }
          else
            {
              filenames.push_back(Filesystem::realpath(argv[i]));
            }
        }

      if (strcmp(argv[1], "view") == 0)
        {
          view(database, filenames);
        }
      else if (strcmp(argv[1], "prepare") == 0)
        {
          generate_tiles(database, filenames);
        }
      else
        {
          print_usage();
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
