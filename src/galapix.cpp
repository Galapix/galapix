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

#include "../config.h"
#include <boost/bind.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>

#include "jpeg.hpp"
#include "png.hpp"
#include "xcf.hpp"
#include "imagemagick.hpp"
#include "surface.hpp"
#include "framebuffer.hpp"
#include "math/size.hpp"
#include "math/rect.hpp"
#include "math/vector2i.hpp"
#include "sqlite.hpp"
#include "software_surface.hpp"
#include "file_database.hpp"
#include "tile_database.hpp"
#include "database_thread.hpp"
#include "filesystem.hpp"
#include "tile_generator.hpp"
#include "tile_generator_thread.hpp"
#include "workspace.hpp"
#include "job_manager.hpp"
#include "sdl_viewer.hpp"
#include "viewer.hpp"
#include "galapix.hpp"

Galapix::Galapix()
  : fullscreen(false),
    geometry(800, 600),
    anti_aliasing(0)
{
  Filesystem::init();
}

Galapix::~Galapix()
{
  Filesystem::deinit();
}

void
Galapix::test(const std::vector<URL>& url)
{
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i)
    {
      std::vector<std::string> layer =  XCF::get_layers(*i);
      std::cout << *i << ":" << std::endl;
      for(std::vector<std::string>::iterator j = layer.begin(); j != layer.end(); ++j)
        {
          std::cout << "  '" << *j << "'" << std::endl;
        }
    }
}

/** Merge content of the databases given by filenames into database */
void
Galapix::merge(const std::string& database,
               const std::vector<std::string>& filenames)
{
  SQLiteConnection db(database);
  FileDatabase out_file_db(&db);
  TileDatabase out_tile_db(&db);

  for(std::vector<std::string>::const_iterator i = filenames.begin(); i != filenames.end(); ++i)
    {
      SQLiteConnection in_db(*i);
      FileDatabase in_file_db(&in_db);
      TileDatabase in_tile_db(&in_db);
          
      std::vector<FileEntry> entries;
      in_file_db.get_file_entries(entries);
      for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
        {
          try {
          std::cout << "Processing: " << i - entries.begin() << "/" << entries.size() << '\r' << std::flush;

          // FIXME: Must catch URL collisions here (or maybe not?)
          FileEntry entry = out_file_db.store_file_entry(*i);

          std::vector<TileEntry> tiles;
          in_tile_db.get_tiles(i->get_fileid(), tiles);
          for(std::vector<TileEntry>::iterator j = tiles.begin(); j != tiles.end(); ++j)
            {
              // Change the fileid
              j->set_fileid(entry.get_fileid());
              out_tile_db.store_tile(*j);
            }
          } catch(std::exception& err) {
            std::cout << "Error: " << err.what() << std::endl;
          }
        }
      std::cout << std::endl;
    }
}

void
Galapix::export_images(const std::string& database, const std::vector<URL>& url)
{
  SQLiteConnection db(database);
  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);
  
  int wish_size = 512;
  int image_num = 0;
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i)
    {
      FileEntry entry = file_db.get_file_entry(*i);
      if (!entry)
        {
          std::cerr << "Error: Couldn't get file entry for " << *i << std::endl;
        }
      else
        {
          Size size = entry.get_image_size();
          int scale = 0;
          while(size.width > wish_size && size.height > wish_size)
            {
              scale += 1;
              size /= 2;
            }

          SoftwareSurface target(SoftwareSurface::RGB_FORMAT, size);
          for(int y = 0; y < (size.height+255)/256; ++y)
            for(int x = 0; x < (size.width+255)/256; ++x)
              {
                TileEntry tile;
                if (tile_db.get_tile(entry.get_fileid(), scale, Vector2i(x, y), tile))
                  {
                    tile.get_surface().blit(target, Vector2i(x, y) * 256);
                  }
              }

          char filename[1024];
          sprintf(filename, "/tmp/out/%04d.jpg", image_num);
          std::cout << "Writing result to: " << filename << std::endl;
          JPEG::save(target, 85, filename);
          image_num += 1;
        }
    }
}

void
Galapix::info(const std::vector<URL>& url)
{
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i)
    {
      Size size;

      if (SoftwareSurface::get_size(*i, size))
        std::cout << *i << " " << size.width << "x" << size.height << std::endl;
      else
        std::cout << "Error reading " << *i << std::endl;
    }
}

void
Galapix::downscale(const std::vector<URL>& url)
{
  int num = 0;
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i, ++num)
    {
      std::cout << *i << std::endl;
      SoftwareSurface surface = JPEG::load_from_file(i->get_stdio_name(), 8);

      std::ostringstream out;
      out << "/tmp/out-" << num << ".jpg";
      Blob blob = JPEG::save(surface, 75);
      blob.write_to_file(out.str());

      std::cout << "Wrote: " << out.str() << std::endl;
    }  
}

void
Galapix::cleanup(const std::string& database)
{
  SQLiteConnection db(database); 
  std::cout << "Running database cleanup routines, this process can take multiple minutes." << std::endl;
  std::cout << "You can interrupt it via Ctrl-c, which won't do harm, but will throw away all the cleanup work done till that point" << std::endl;
  db.vacuum();
  std::cout << "Running database cleanup routines done" << std::endl;
}

void
Galapix::list(const std::string& database)
{
  SQLiteConnection db(database);

  FileDatabase file_db(&db);

  std::vector<FileEntry> entries;
  file_db.get_file_entries(entries);

  for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
    {
      std::cout << i->get_url() << std::endl;
    }  
}

void
Galapix::check(const std::string& database)
{
  SQLiteConnection db(database);

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  file_db.check();
  tile_db.check();
}

void
Galapix::filegen(const std::string& database, 
                 const std::vector<URL>& url)
{
  SQLiteConnection db(database);
  FileDatabase file_db(&db);  

  for(std::vector<URL>::size_type i = 0; i < url.size(); ++i)
    {
      FileEntry entry = file_db.get_file_entry(url[i]);
      if (!entry)
        {
          std::cout << "Couldn't get entry for " << url[i] << std::endl;
        }
      else
        {
          std::cout << "Got: " << entry.get_url() << " " << entry.get_width() << "x" << entry.get_height() << std::endl;
        }
    }
}

void
Galapix::thumbgen(const std::string& database, 
                  const std::vector<URL>& url)
{
  SQLiteConnection db(database);

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  TileGenerator tile_generator;

  for(std::vector<std::string>::size_type i = 0; i < url.size(); ++i)
    {
      try 
        {
          FileEntry entry = file_db.get_file_entry(url[i]);
          // FIXME: Insert some checks if the tile already exist

          // Generate Image Tiles
          std::cout << "Generating tiles for " << url[i]  << std::endl;
          if (entry)
            {
              try {
                tile_generator.generate_quick(entry,
                                              boost::bind(&TileDatabase::store_tile, &tile_db, _1));
              } catch(std::exception& err) {
                std::cout << err.what() << std::endl;
              }
            }
        }
      catch (std::exception& err) 
        {
          std::cout << "Couldn't find entry for " << url[i] << std::endl;
        }
    }
}

void
Galapix::generate_tiles(const std::string& database, 
                        const std::vector<URL>& urls)
{
  SQLiteConnection db(database);

  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);

  TileGenerator tile_generator;

  for(std::vector<URL>::size_type i = 0; i < urls.size(); ++i)
    {
      std::cout << "Getting file entry..." << std::endl;
      FileEntry entry = file_db.get_file_entry(urls[i]);
      if (!entry)
        {
          std::cout << "Couldn't find entry for " << urls[i] << std::endl;
        }
      else
        {
          // Generate Image Tiles
          std::cout << "Generating tiles... " << urls[i]  << std::endl;
          SoftwareSurface surface = SoftwareSurface::from_url(urls[i]);
          
          tile_generator.generate_all(entry.get_fileid(), surface, 
                                      boost::bind(&TileDatabase::store_tile, &tile_db, _1));
        }
    }
}

void
Galapix::view(const std::string& database, 
              const std::vector<URL>& urls, 
              bool view_all, 
              const std::string& pattern)
{
  JobManager job_manager(4);
  DatabaseThread      database_thread(database);
  TileGeneratorThread tile_generator_thread;

  database_thread.start();
  tile_generator_thread.start();

  Workspace workspace;
  SDLViewer sdl_viewer(geometry, fullscreen, anti_aliasing);

  sdl_viewer.set_workspace(&workspace);

  if (view_all)
    {
      if (pattern.empty())
        {
          // When no files are given, display everything in the database
          database_thread.request_all_files(boost::bind(&SDLViewer::receive_file, &sdl_viewer, _1));
        }
      else 
        {
          std::cout << "Using pattern: '" << pattern << "'" << std::endl;
          database_thread.request_files_by_pattern(boost::bind(&SDLViewer::receive_file, &sdl_viewer, _1), pattern);
        }
    }

  for(std::vector<URL>::const_iterator i = urls.begin(); i != urls.end(); ++i)
    {
      if (i->has_stdio_name() && Filesystem::has_extension(i->get_stdio_name(), ".galapix"))
        {
          workspace.load(i->get_stdio_name());
        }
      else
        {
          database_thread.request_file(*i, boost::bind(&SDLViewer::receive_file, &sdl_viewer, _1));
        }
    }
  
  sdl_viewer.run();

  tile_generator_thread.stop();
  database_thread.stop();
  
  tile_generator_thread.join(); 
  database_thread.join();
}

void
Galapix::print_usage()
{
  std::cout << "Usage: galapix view     [OPTIONS]... [FILES]...\n"
            << "       galapix viewdb   [OPTIONS]... [FILES]...\n"
            << "       galapix prepare  [OPTIONS]... [FILES]...\n"
            << "       galapix thumbgen [OPTIONS]... [FILES]...\n"
            << "       galapix filegen  [OPTIONS]... [FILES]...\n"
            << "       galapix info     [OPTIONS]... [FILES]...\n"
            << "       galapix check    [OPTIONS]...\n"
            << "       galapix list     [OPTIONS]...\n"
            << "       galapix cleanup  [OPTIONS]...\n"
            << "       galapix merge    [OPTIONS]... [FILES]...\n"
            << "\n"
            << "Commands:\n"
            << "  view      Display the given files\n"
            << "  viewdb    Display all files in the database\n"
            << "  prepare   Generate all thumbnail for all given images\n"
            << "  thumbgen  Generate only small thumbnails for all given images\n"
            << "  filegen   Generate only small the file entries in the database\n"
            << "  list      Lists all files in the database\n"
            << "  check     Checks the database for consistency\n"
            << "  info      Display size of the given files\n"
            << "  cleanup   Runs garbage collection on the database\n"
            << "  merge     Merges the given databases into the database given by -d FILE\n"
            << "\n"
            << "Options:\n"
            << "  -d, --database FILE    Use FILE has database (default: none)\n"
            << "  -f, --fullscreen       Start in fullscreen mode\n"
            << "  -F, --files-from FILE  Get urls from FILE\n"
            << "  -p, --pattern GLOB     Select files from the database via globbing pattern\n"
            << "  -g, --geometry WxH     Start with window size WxH\n"        
            << "  -a, --anti-aliasing N  Anti-aliasing factor 0,2,4 (default: 0)\n"
            << "\n"
            << "If you do not supply any files, the whole content of the given database will be displayed.\n"
            << "\n"
            << "Compiled Fetures:\n" 
#ifdef HAVE_SPACE_NAVIGATOR
            << "  * SpaceNavigator: enabled\n"
#else
            << "  * SpaceNavigator: disabled\n"
#endif
            << std::endl;
}

int
Galapix::main(int argc, char** argv)
{
  // FIXME: Function doesn't seem to be available in 3.4.2
  // if (!sqlite3_threadsafe())
  //  throw std::runtime_error("Error: SQLite must be compiled with SQLITE_THREADSAFE");

  std::string database = Filesystem::get_home() + "/.galapix/cache.sqlite";
  
  if (argc < 2)
    {
      print_usage();
    }
  else
    {
      std::string pattern;
      std::vector<std::string> argument_filenames;
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
              else if (strcmp(argv[i], "-F") == 0 ||
                       strcmp(argv[i], "--files-from") == 0)
                {
                  ++i;
                  if (i < argc)
                    {
                      std::string line;
                      std::ifstream in(argv[i]);
                      if (!in)
                        {
                          throw std::runtime_error("Couldn't open " + std::string(argv[i]));
                        }
                      else
                        {
                          while(std::getline(in, line))
                            {
                              argument_filenames.push_back(line);
                            }
                        }
                    }
                  else
                    {
                      throw std::runtime_error(std::string(argv[i-1]) + " requires an argument");
                    }
                }
              else if (strcmp(argv[i], "--pattern") == 0 ||
                       strcmp(argv[i], "-p") == 0)
                {
                  i += 1;
                  if (i < argc)
                    pattern = argv[i];
                  else
                    throw std::runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
                }
              else if (strcmp(argv[i], "--anti-aliasing") == 0 ||
                       strcmp(argv[i], "-a") == 0)
                {
                  i += 1;
                  if (i < argc)
                    anti_aliasing = atoi(argv[i]);
                  else
                    throw std::runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");                  
                }
              else if (strcmp(argv[i], "--geometry") == 0 ||
                       strcmp(argv[i], "-g") == 0)
                {
                  i += 1;
                  if (i < argc)
                    sscanf(argv[i], "%dx%d", &geometry.width, &geometry.height);
                  else
                    throw std::runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
                }
              else if (strcmp(argv[i], "--fullscreen") == 0 ||
                       strcmp(argv[i], "-f") == 0)
                {
                  fullscreen = true;
                }
              else
                {
                  throw std::runtime_error("Unknown option " + std::string(argv[i]));
                }
            }
          else
            {
              argument_filenames.push_back(argv[i]);
            }
        }

      std::cout << "Using database: " << (database.empty() ? "memory" : database) << std::endl;

      std::vector<URL> urls;

      std::cout << "Scanning directories... " << std::flush;
      for(std::vector<std::string>::iterator i = argument_filenames.begin(); i != argument_filenames.end(); ++i)
        {
          if (URL::is_url(*i))
            urls.push_back(URL::from_string(*i));
          else
            Filesystem::generate_image_file_list(*i, urls);
        }
      std::sort(urls.begin(), urls.end());
      std::cout << urls.size() << " files found." << std::endl;

      if (strcmp(argv[1], "view") == 0)
        {
          if (!urls.empty())
            view(database, urls, false, pattern);
          else
            std::cout << "Error: No URLs given" << std::endl;
        }
      else if (strcmp(argv[1], "viewdb") == 0)
        {
          view(database, urls, true, pattern);
        }
      else if (strcmp(argv[1], "check") == 0)
        {
          check(database);
        }
      else if (strcmp(argv[1], "list") == 0)
        {
          list(database);
        }
      else if (strcmp(argv[1], "cleanup") == 0)
        {
          cleanup(database);
        }
      else if (strcmp(argv[1], "export") == 0)
        {
          export_images(database, urls);
        }
      else if (strcmp(argv[1], "merge") == 0)
        {
          merge(database, argument_filenames);
        }
      else if (strcmp(argv[1], "info") == 0)
        {
          info(urls);
        }
      else if (strcmp(argv[1], "test") == 0)
        {
          test(urls);
        }
      else if (strcmp(argv[1], "downscale") == 0)
        {
          downscale(urls);
        }
      else if (strcmp(argv[1], "prepare") == 0)
        {
          generate_tiles(database, urls);
        }
      else if (strcmp(argv[1], "thumbgen") == 0)
        {
          thumbgen(database, urls);
        }
      else if (strcmp(argv[1], "filegen") == 0)
        {
          filegen(database, urls);
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
      Galapix app;
      int ret = app.main(argc, argv);
      return ret;
    }
  catch(const std::exception& err) 
    {
      std::cout << "Exception: " << err.what() << std::endl;
      return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
  
/* EOF */
