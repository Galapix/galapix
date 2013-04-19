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

#include "galapix/galapix.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include "archive/archive_manager.hpp"
#include "database/database.hpp"
#include "display/framebuffer.hpp"
#include "display/surface.hpp"
#include "galapix/database_thread.hpp"
#include "galapix/database_tile_provider.hpp"
#include "galapix/mandelbrot_tile_provider.hpp"
#include "galapix/options.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "galapix/zoomify_tile_provider.hpp"
#include "job/job_handle_group.hpp"
#include "job/job_manager.hpp"
#include "jobs/test_job.hpp"
#include "jobs/tile_generation_job.hpp"
#include "math/rect.hpp"
#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "network/download_manager.hpp"
#include "plugins/imagemagick.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "plugins/xcf.hpp"
#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"
#include "util/string_util.hpp"
#ifdef GALAPIX_SDL
#  include "sdl/sdl_viewer.hpp"
#endif
#ifdef GALAPIX_GTK
#  include "gtk/gtk_viewer.hpp"
#endif

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
Galapix::export_images(const std::string& database, const std::vector<URL>& url)
{
#if 0
  Database db(database);
  
  int wish_size = 512;
  int image_num = 0;
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i)
  {
    FileEntry entry = db.get_resources().get_file_entry(*i);
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

      SoftwareSurfacePtr target = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT, size);
      for(int y = 0; y < (size.height+255)/256; ++y)
        for(int x = 0; x < (size.width+255)/256; ++x)
        {
          TileEntry tile;
          if (db.get_tiles().get_tile(entry.get_fileid(), scale, Vector2i(x, y), tile))
          {
            tile.get_surface()->blit(target, Vector2i(x, y) * 256);
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
#endif
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
Galapix::list(const Options& opts)
{
  Database db(opts.database);

  std::vector<OldFileEntry> entries;
  if (opts.patterns.empty())
  {
    db.get_resources().get_old_file_entries(entries);
  }
  else
  {
    for(std::vector<std::string>::const_iterator i = opts.patterns.begin(); i != opts.patterns.end(); ++i)
    {
      db.get_resources().get_old_file_entries(*i, entries);
    }
  }

  for(std::vector<OldFileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
  {
    std::cout << i->get_url() << std::endl;
  }  
}

void
Galapix::thumbgen(const Options& opts,
                  const std::vector<URL>& urls, 
                  bool generate_all_tiles)
{
  Database       database(opts.database);
  JobManager     job_manager(opts.threads);
  DatabaseThread database_thread(database, job_manager);
  
  database_thread.start_thread();
  job_manager.start_thread();

  std::vector<OldFileEntry> file_entries;

  JobHandleGroup job_handle_group;

  // gather FileEntries
  for(std::vector<URL>::const_iterator i = urls.begin(); i != urls.end(); ++i)
  {
    job_handle_group.add(database_thread.request_file(*i, 
                                                      [&file_entries](const OldFileEntry& entry) { 
                                                        file_entries.push_back(entry); 
                                                      }));
  }
  job_handle_group.wait();
  job_handle_group.clear();

  std::cout << "Got " << file_entries.size() << " files, generating tiles...: "  << generate_all_tiles << std::endl;

  // gather thumbnails
  for(std::vector<OldFileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
  {
    int min_scale = 0;
    int max_scale = 1; // FIXME: i->get_thumbnail_scale();

    if (!generate_all_tiles)
    {
      min_scale = std::max(0, max_scale - 3);
    }

    job_handle_group.add(database_thread.request_tiles(*i, min_scale, max_scale,
                                                       std::function<void(Tile)>()));
  }

  job_handle_group.wait();
  job_handle_group.clear();

  job_manager.stop_thread();
  database_thread.stop_thread();

  job_manager.join_thread();
  database_thread.join_thread();
}

void
Galapix::view(const Options& opts, const std::vector<URL>& urls)
{
  Database       database(opts.database);
  JobManager     job_manager(opts.threads);

  Workspace workspace;

  { // process all -p PATTERN options 
    std::vector<OldFileEntry> file_entries;

    for(std::vector<std::string>::const_iterator i = opts.patterns.begin(); i != opts.patterns.end(); ++i)
    {
      std::cout << "Processing pattern: '" << *i << "'" << std::endl;

      if (*i == "*")
      {
        // special case to display everything, might be faster then
        // using the pattern
        database.get_resources().get_old_file_entries(file_entries);
      }
      else
      {
        database.get_resources().get_old_file_entries(*i, file_entries);
      }
    }

    for(std::vector<OldFileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
    {
      ImageEntry image_entry;
      if (!database.get_resources().get_image_entry(*i, image_entry))
      {
        log_warn("no ImageEntry for " << i->get_url());
      }
      else
      {
        WorkspaceItemPtr image(new Image(i->get_url(), std::make_shared<DatabaseTileProvider>(*i, image_entry)));
        workspace.add_image(image);

        // print progress
        int n = (i - file_entries.begin())+1;
        int total = file_entries.size();
        std::cout << "Getting tiles: " << n << "/" << total << " - "
                  << (100 * n / total) << '%'
                  << '\r' << std::flush;
      }
    }

    if (!file_entries.empty())
    {
      std::cout << std::endl;
    }
  }

  // process regular URLs
  for(std::vector<URL>::const_iterator i = urls.begin(); i != urls.end(); ++i)
  {
    int n = (i - urls.begin())+1;
    int total = urls.size();
    std::cout << "Processing URLs: " << n << "/" << total << " - " << (100 * n / total) << "%\r" << std::flush;

    if (i->has_stdio_name() && Filesystem::has_extension(i->get_stdio_name(), ".galapix"))
    {
      // FIXME: Right place for this?
      workspace.load(i->get_stdio_name());
    }
    else if (i->get_protocol() == "buildin")
    {
      if (i->get_payload() == "mandelbrot")
      {
        workspace.add_image(WorkspaceItemPtr(new Image(*i, TileProviderPtr(new MandelbrotTileProvider(job_manager)))));
      }
      else
      {
        std::cout << "Galapix::view(): unknown buildin:// requested: " << *i << " ignoring" << std::endl;
      }
    }
    else if (Filesystem::has_extension(i->str(), "ImageProperties.xml"))
    {
      workspace.add_image(WorkspaceItemPtr(new Image(*i, ZoomifyTileProvider::create(*i, job_manager))));
    }
    else
    {
      OldFileEntry file_entry;
      if (!database.get_resources().get_old_file_entry(*i, file_entry))
      {
        workspace.add_image(WorkspaceItemPtr(new Image(*i)));
      }
      else
      {
        ImageEntry image_entry;
        if (!database.get_resources().get_image_entry(file_entry, image_entry))
        {
          log_warn("no ImageEntry for " << *i);
        }
        else
        {
          WorkspaceItemPtr image(new Image(file_entry.get_url(), std::make_shared<DatabaseTileProvider>(file_entry, image_entry)));
          workspace.add_image(image);
        }
      }
    }
  }
  
  if (!urls.empty())
  {
    std::cout << std::endl;
  }

  DatabaseThread database_thread(database, job_manager);
  job_manager.start_thread();  
  database_thread.start_thread();

#ifdef GALAPIX_SDL
  Viewer viewer(&workspace);
  SDLViewer sdl_viewer(geometry, fullscreen, anti_aliasing, viewer);
  viewer.layout_tight();
  viewer.zoom_to_selection();
  sdl_viewer.run();
#endif

#ifdef GALAPIX_GTK
  GtkViewer gtk_viewer;
  gtk_viewer.set_workspace(&workspace);
  gtk_viewer.run();
#endif

  job_manager.abort_thread();
  database_thread.abort_thread();

  job_manager.join_thread();
  database_thread.join_thread();
}

void
Galapix::print_usage()
{
  std::cout << "Usage: galapix view     [OPTIONS]... [FILES]...\n"
            << "       galapix thumbgen [OPTIONS]... [FILES]...\n"
            << "       galapix list     [OPTIONS]...\n"
            << "       galapix cleanup  [OPTIONS]...\n"
            << "\n"
            << "Commands:\n"
            << "  view      Display the given files\n"
            << "  thumbgen  Generate only small thumbnails for all given images\n"
            << "  list      Lists all files in the database\n"
            << "  cleanup   Runs garbage collection on the database\n"
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
  //  raise_runtime_error("Error: SQLite must be compiled with SQLITE_THREADSAFE");

  try 
  {
    Options opts;
    opts.threads  = 2;
    opts.database = Filesystem::get_home() + "/.galapix/cache4";
    parse_args(argc, argv, opts);

    DownloadManager download_manager;
    ArchiveManager archive_manager;
    SoftwareSurfaceFactory software_surface_factory;

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
Galapix::run(const Options& opts)
{
  std::cout << "Using database: " << (opts.database.empty() ? "memory" : opts.database) << std::endl;

  if (opts.rest.empty())
  {
#ifdef GALAPIX_SDL
    print_usage();
#else
    view(opts, std::vector<URL>());
#endif
  }
  else
  {
    std::vector<URL> urls;

    std::cout << "Scanning directories... " << std::flush;
    for(std::vector<std::string>::const_iterator i = opts.rest.begin()+1; i != opts.rest.end(); ++i)
    {
      if (URL::is_url(*i))
        urls.push_back(URL::from_string(*i));
      else
        Filesystem::generate_image_file_list(*i, urls);
    }
    std::sort(urls.begin(), urls.end(),
              [](const URL& lhs, const URL& rhs) {
                return StringUtil::numeric_less(lhs.str(), rhs.str());
              });
    std::cout << urls.size() << " files found." << std::endl;

    const std::string& command = opts.rest.front();
        
    if (command == "view")
    {
      if (urls.empty() && opts.patterns.empty())
      {
        std::cout << "Galapix::run(): Error: No URLs given" << std::endl;
      }
      else
      {
        view(opts, urls);
      }
    }
    else if (command == "list")
    {
      list(opts);
    }
    else if (command == "cleanup")
    {
      cleanup(opts.database);
    }
    else if (command == "export")
    {
      export_images(opts.database, urls);
    }
    else if (command == "thumbgen")
    {
      thumbgen(opts, urls, false);
    }
    else
    {
      print_usage();
    }
  }
}

void
Galapix::parse_args(int argc, char** argv, Options& opts)
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
          raise_runtime_error(std::string(argv[i-1]) + " requires an argument");
        }
      }
      else if (strcmp(argv[i], "-D") == 0 ||
               strcmp(argv[i], "--debug") == 0)
      {
        g_logger.set_log_level(Logger::kDebug);
      }
      else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0)
      {
        g_logger.set_log_level(Logger::kInfo);
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
          raise_runtime_error(std::string(argv[i-1]) + " requires an argument");
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
            raise_runtime_error("Couldn't open " + std::string(argv[i]));
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
          raise_runtime_error(std::string(argv[i-1]) + " requires an argument");
        }
      }
      else if (strcmp(argv[i], "--pattern") == 0 ||
               strcmp(argv[i], "-p") == 0)
      {
        i += 1;
        if (i < argc)
          opts.patterns.push_back(argv[i]);
        else
          raise_runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
      }
      else if (strcmp(argv[i], "--anti-aliasing") == 0 ||
               strcmp(argv[i], "-a") == 0)
      {
        i += 1;
        if (i < argc)
          anti_aliasing = atoi(argv[i]);
        else
          raise_runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");                  
      }
      else if (strcmp(argv[i], "--geometry") == 0 ||
               strcmp(argv[i], "-g") == 0)
      {
        i += 1;
        if (i < argc)
          sscanf(argv[i], "%dx%d", &geometry.width, &geometry.height);
        else
          raise_runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
      }
      else if (strcmp(argv[i], "--fullscreen") == 0 ||
               strcmp(argv[i], "-f") == 0)
      {
        fullscreen = true;
      }
      else
      {
        raise_runtime_error("Unknown option " + std::string(argv[i]));
      }
    }
    else
    {
      opts.rest.push_back(argv[i]);
    }
  }
}

/* EOF */
