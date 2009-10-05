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

#include <boost/bind.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "plugins/xcf.hpp"
#include "plugins/imagemagick.hpp"
#include "surface.hpp"
#include "framebuffer.hpp"
#include "math/size.hpp"
#include "math/rect.hpp"
#include "math/vector2i.hpp"
#include "software_surface.hpp"
#include "database.hpp"
#include "jobs/tile_generation_job.hpp"
#include "jobs/test_job.hpp"
#include "database_thread.hpp"
#include "filesystem.hpp"
#include "workspace.hpp"
#include "job_manager.hpp"
#ifdef GALAPIX_SDL
#  include "sdl_viewer.hpp"
#endif
#ifdef GALAPIX_GTK
#  include "gtk_viewer.hpp"
#endif
#include "viewer.hpp"
#include "galapix.hpp"

struct GalapixOptions
{
  std::string database;
  std::string pattern;
  int         threads;
  std::vector<std::string> rest;
};

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
Galapix::test(const GalapixOptions& opts,
              const std::vector<URL>& url)
{
  std::cout << "Running test case" << std::endl;

  DatabaseThread database_thread(opts.database);
  JobManager job_manager(2);

  database_thread.start_thread();
  job_manager.start_thread();

  std::cout << "<<<--- launching jobs" << std::endl;
  JobHandle handle1 = job_manager.request(boost::shared_ptr<Job>(new TestJob()),
                                          boost::function<void (boost::shared_ptr<Job>)>());
  JobHandle handle2 = job_manager.request(boost::shared_ptr<Job>(new TestJob()), 
                                          boost::function<void (boost::shared_ptr<Job>)>());
  std::cout << "--->>> waiting for jobs" << std::endl;
  handle1.wait();
  std::cout << "handle1 finished" << std::endl;
  handle2.wait();
  std::cout << "handle2 finished" << std::endl;
  std::cout << "--->>> waiting for jobs DONE" << std::endl;

  database_thread.stop_thread();
  job_manager.stop_thread();

  database_thread.join_thread();
  job_manager.join_thread();
}

/** Merge content of the databases given by filenames into database */
void
Galapix::merge(const std::string& database,
               const std::vector<std::string>& filenames)
{
  Database out_db(database);

  for(std::vector<std::string>::const_iterator i = filenames.begin(); i != filenames.end(); ++i)
    {
      Database in_db(*i);
          
      std::vector<FileEntry> entries;
      in_db.files.get_file_entries(entries);
      for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
        {
          try {
            std::cout << "Processing: " << i - entries.begin() << "/" << entries.size() << '\r' << std::flush;

            // FIXME: Must catch URL collisions here (or maybe not?)
            FileEntry entry = out_db.files.store_file_entry(*i);

            std::vector<TileEntry> tiles;
            in_db.tiles.get_tiles(i->get_fileid(), tiles);
            for(std::vector<TileEntry>::iterator j = tiles.begin(); j != tiles.end(); ++j)
              {
                // Change the fileid
                j->set_fileid(entry.get_fileid());
                out_db.tiles.store_tile_in_cache(*j);
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
  Database db(database);
  
  int wish_size = 512;
  int image_num = 0;
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i)
    {
      FileEntry entry = db.files.get_file_entry(*i);
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
                if (db.tiles.get_tile(entry.get_fileid(), scale, Vector2i(x, y), tile))
                  {
                    tile.get_surface().blit(target, Vector2i(x, y) * 256);
                  }
              }

          char filename[1024];
          sprintf(filename, "/tmp/out/%04d.jpg", image_num);
          std::cout << "Writing result to: " << filename << std::endl;
          JPEG::save(target, 85, filename);
          //PNG::save(target, filename);
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
  Database db(database); 
  std::cout << "Running database cleanup routines, this process can take multiple minutes." << std::endl;
  std::cout << "You can interrupt it via Ctrl-c, which won't do harm, but will throw away all the cleanup work done till that point" << std::endl;
  db.cleanup();
  std::cout << "Running database cleanup routines done" << std::endl;
}

void
Galapix::list(const std::string& database, const std::string& pattern)
{
  Database db(database);

  std::vector<FileEntry> entries;
  if (pattern.empty())
    db.files.get_file_entries(entries);
  else
    db.files.get_file_entries(entries, pattern);

  for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
    {
      std::cout << i->get_url() << std::endl;
    }  
}

void
Galapix::check(const std::string& database)
{
  Database db(database);
  db.files.check();
  db.tiles.check();
}

void
Galapix::filegen(const std::string& database, 
                 const std::vector<URL>& url)
{
  Database db(database);
  
  for(std::vector<URL>::size_type i = 0; i < url.size(); ++i)
    {
      FileEntry entry = db.files.get_file_entry(url[i]);
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
Galapix::thumbgen(const GalapixOptions& opts,
                  const std::vector<URL>& urls, 
                  bool generate_all_tiles)
{
  DatabaseThread database_thread(opts.database);
  JobManager     job_manager(opts.threads);

  database_thread.start_thread();
  job_manager.start_thread();

  std::vector<FileEntry> file_entries;
  {
    std::vector<JobHandle> job_handles;
    for(std::vector<URL>::const_iterator i = urls.begin(); i != urls.end(); ++i)
      job_handles.push_back(database_thread.request_file(*i, boost::bind(&std::vector<FileEntry>::push_back, &file_entries, _1))); 
    for(std::vector<JobHandle>::iterator i = job_handles.begin(); i != job_handles.end(); ++i)
      i->wait();
  }

  {
    std::vector<JobHandle> job_handles;
    for(std::vector<FileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
      {
        // Generate Image Tiles
        SoftwareSurface surface = SoftwareSurface::from_url(i->get_url());
        
        JobHandle job_handle;
        boost::shared_ptr<Job> job_ptr(new TileGenerationJob(job_handle, *i, 0, i->get_thumbnail_scale(),
                                                             boost::bind(&DatabaseThread::receive_tile, &database_thread, _1)));
        job_manager.request(job_ptr);
        job_handles.push_back(job_handle);
      }

    for(std::vector<JobHandle>::iterator i = job_handles.begin(); i != job_handles.end(); ++i)
      i->wait();
  }

  job_manager.stop_thread();
  database_thread.stop_thread();

  job_manager.join_thread();
  database_thread.join_thread();
}

void
Galapix::view(const GalapixOptions& opts,
              const std::vector<URL>& urls, 
              bool view_all, 
              const std::string& pattern)
{
  DatabaseThread database_thread(opts.database);
  JobManager     job_manager(opts.threads);

  database_thread.start_thread();
  job_manager.start_thread();

  Workspace workspace;

  if (view_all)
    {
      if (pattern.empty())
        {
          // When no files are given, display everything in the database
          database_thread.request_all_files(boost::bind(&Workspace::receive_file, &workspace, _1));
        }
      else 
        {
          std::cout << "Using pattern: '" << pattern << "'" << std::endl;
          database_thread.request_files_by_pattern(boost::bind(&Workspace::receive_file, &workspace, _1), pattern);
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
          database_thread.request_file(*i, boost::bind(&Workspace::receive_file, &workspace, _1));
        }
    }

#ifdef GALAPIX_SDL
  SDLViewer sdl_viewer(geometry, fullscreen, anti_aliasing);
  sdl_viewer.set_workspace(&workspace);
  sdl_viewer.run();  
#endif

#ifdef GALAPIX_GTK
  GtkViewer gtk_viewer;
  gtk_viewer.set_workspace(&workspace);
  gtk_viewer.run();
#endif

  job_manager.stop_thread();
  database_thread.stop_thread();

  job_manager.join_thread();
  database_thread.join_thread();
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
            << "  -t, --threads          Number of worker threads (default: 2)\n"
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

  try 
    {
      GalapixOptions opts;
      opts.threads  = 2;
      opts.database = Filesystem::get_home() + "/.galapix/cache.sqlite";
      parse_args(argc, argv, opts);
      run(opts);
      return EXIT_SUCCESS;
    }
  catch(const std::exception& err) 
    {
      std::cout << "Exception: " << err.what() << std::endl;
      return EXIT_FAILURE;
    }
}
  
void
Galapix::run(const GalapixOptions& opts)
{
  std::cout << "Using database: " << (opts.database.empty() ? "memory" : opts.database) << std::endl;

  std::vector<URL> urls;

  if (opts.rest.empty())
    {
#ifdef GALAPIX_SDL
      print_usage();
#else
      view(opts, std::vector<URL>(), false, "");
#endif
    }
  else
    {
      std::cout << "Scanning directories... " << std::flush;
      for(std::vector<std::string>::const_iterator i = opts.rest.begin()+1; i != opts.rest.end(); ++i)
        {
          if (URL::is_url(*i))
            urls.push_back(URL::from_string(*i));
          else
            Filesystem::generate_image_file_list(*i, urls);
        }
      std::sort(urls.begin(), urls.end());
      std::cout << urls.size() << " files found." << std::endl;

      const std::string& command = opts.rest.front();
        
      if (command == "view")
        {
          if (!urls.empty())
            view(opts, urls, false, opts.pattern);
          else
            std::cout << "Error: No URLs given" << std::endl;
        }
      else if (command == "viewdb")
        {
          view(opts, urls, true, opts.pattern);
        }
      else if (command == "check")
        {
          check(opts.database);
        }
      else if (command == "list")
        {
          list(opts.database, opts.pattern);
        }
      else if (command == "cleanup")
        {
          cleanup(opts.database);
        }
      else if (command == "export")
        {
          export_images(opts.database, urls);
        }
      else if (command == "merge")
        {
          merge(opts.database, std::vector<std::string>(opts.rest.begin()+1, opts.rest.end()));
        }
      else if (command == "info")
        {
          info(urls);
        }
      else if (command == "test")
        {
          test(opts, urls);
        }
      else if (command == "downscale")
        {
          downscale(urls);
        }
      else if (command == "prepare")
        {
          thumbgen(opts, urls, true);
        }
      else if (command == "thumbgen")
        {
          thumbgen(opts, urls, false);
        }
      else if (command == "filegen")
        {
          filegen(opts.database, urls);
        }
      else
        {
          print_usage();
        }
    }
}

void
Galapix::parse_args(int argc, char** argv, GalapixOptions& opts)
{
  // Parse arguments
  for(int i = 1; i < argc; ++i)
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
                  opts.database = argv[i];
                }
              else
                {
                  throw std::runtime_error(std::string(argv[i-1]) + " requires an argument");
                }
            }
          else if (strcmp(argv[i], "-t") == 0 ||
                   strcmp(argv[i], "--threads") == 0)
            {
              ++i;
              if (i < argc)
                {
                  opts.threads = atoi(argv[i]);
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
                          opts.rest.push_back(line);
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
                opts.pattern = argv[i];
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
          opts.rest.push_back(argv[i]);
        }
    }
}
  
int main(int argc, char** argv)
{
  Galapix app;
  return app.main(argc, argv);
}  

/* EOF */
