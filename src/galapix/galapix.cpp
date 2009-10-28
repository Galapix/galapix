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

#include <boost/bind.hpp>
#include <curl/curl.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include "database/database.hpp"
#include "display/framebuffer.hpp"
#include "display/surface.hpp"
#include "galapix/database_thread.hpp"
#include "galapix/options.hpp"
#include "galapix/viewer.hpp"
#include "galapix/workspace.hpp"
#include "galapix/zoomify_tile_provider.hpp"
#include "galapix/mandelbrot_tile_provider.hpp"
#include "galapix/database_tile_provider.hpp"
#include "job/job_handle_group.hpp"
#include "job/job_manager.hpp"
#include "jobs/test_job.hpp"
#include "jobs/tile_generation_job.hpp"
#include "math/rect.hpp"
#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "plugins/imagemagick.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "plugins/xcf.hpp"
#include "util/filesystem.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"
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
Galapix::test(const Options& opts,
              const std::vector<URL>& url)
{
  std::cout << "Running test case" << std::endl;

  Database database(opts.database);
  JobManager job_manager(opts.threads);
  DatabaseThread database_thread(database, job_manager);

  database_thread.start_thread();
  job_manager.start_thread();

  std::cout << "<<<--- launching jobs" << std::endl;
  JobHandle handle1 = job_manager.request(boost::shared_ptr<Job>(new TestJob()));
  JobHandle handle2 = job_manager.request(boost::shared_ptr<Job>(new TestJob()));
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

  for(std::vector<std::string>::const_iterator db_it = filenames.begin(); db_it != filenames.end(); ++db_it)
  {
    Database in_db(*db_it);
          
    std::vector<FileEntry> entries;
    in_db.files.get_file_entries(entries);
    for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
    {
      try {
        std::cout << "Processing: " << i - entries.begin() << "/" << entries.size() << '\r' << std::flush;

        // FIXME: Must catch URL collisions here (or maybe not?)
        FileEntry file_entry = out_db.files.store_file_entry(*i);

        std::vector<TileEntry> tiles;
        in_db.tiles.get_tiles(*i, tiles);
        for(std::vector<TileEntry>::iterator j = tiles.begin(); j != tiles.end(); ++j)
        {
          // Change the fileid
          j->set_file_entry(file_entry);
          out_db.tiles.store_tile(file_entry, *j);
        }
      } catch(std::exception& err) {
        std::cout << "Galapix:merge: Error: " << err.what() << std::endl;
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

      SoftwareSurfacePtr target = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT, size);
      for(int y = 0; y < (size.height+255)/256; ++y)
        for(int x = 0; x < (size.width+255)/256; ++x)
        {
          TileEntry tile;
          if (db.tiles.get_tile(entry, scale, Vector2i(x, y), tile))
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
}

void
Galapix::info(const std::vector<URL>& url)
{
  for(std::vector<URL>::const_iterator i = url.begin(); i != url.end(); ++i)
  {
    Size size;

    if (SoftwareSurfaceFactory::get_size(*i, size))
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
    SoftwareSurfacePtr surface = JPEG::load_from_file(i->get_stdio_name(), 8);

    std::ostringstream out;
    out << "/tmp/out-" << num << ".jpg";
    BlobPtr blob = JPEG::save(surface, 75);
    blob->write_to_file(out.str());

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
Galapix::list(const Options& opts)
{
  Database db(opts.database);

  std::vector<FileEntry> entries;
  if (opts.patterns.empty())
  {
    db.files.get_file_entries(entries);
  }
  else
  {
    for(std::vector<std::string>::const_iterator i = opts.patterns.begin(); i != opts.patterns.end(); ++i)
    {
      db.files.get_file_entries(*i, entries);
    }
  }

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
Galapix::filegen(const Options& opts,
                 const std::vector<URL>& url)
{
  Database database(opts.database);
  JobManager job_manager(opts.threads);
  DatabaseThread database_thread(database, job_manager);

  job_manager.start_thread();
  database_thread.start_thread();
  
  for(std::vector<URL>::size_type i = 0; i < url.size(); ++i)
  {
    database_thread.request_file(url[i], 
                                 boost::function<void (FileEntry)>(), 
                                 boost::function<void (FileEntry, Tile)>());
  }

  job_manager.stop_thread();
  database_thread.stop_thread();

  job_manager.join_thread();
  database_thread.join_thread();
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

  std::vector<FileEntry> file_entries;

  JobHandleGroup job_handle_group;

  // gather FileEntries
  for(std::vector<URL>::const_iterator i = urls.begin(); i != urls.end(); ++i)
  {
    job_handle_group.add(database_thread.request_file(*i, 
                                                      boost::bind(&std::vector<FileEntry>::push_back, &file_entries, _1),
                                                      boost::function<void (FileEntry, Tile)>())); 
  }
  job_handle_group.wait();
  job_handle_group.clear();

  std::cout << "Got " << file_entries.size() << " files, generating tiles...: "  << generate_all_tiles << std::endl;

  // gather thumbnails
  for(std::vector<FileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
  {
    int min_scale = 0;
    int max_scale = i->get_thumbnail_scale();

    if (!generate_all_tiles)
    {
      min_scale = std::max(0, max_scale - 3);
    }

    job_handle_group.add(database_thread.request_tiles(*i, min_scale, max_scale,
                                                       boost::function<void(Tile)>()));
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
  DatabaseThread database_thread(database, job_manager);

  Workspace workspace;

  { // process all -p PATTERN options 
    std::vector<FileEntry> file_entries;

    for(std::vector<std::string>::const_iterator i = opts.patterns.begin(); i != opts.patterns.end(); ++i)
    {
      std::cout << "Processing pattern: '" << *i << "'" << std::endl;

      if (*i == "*")
      { 
        // special case to display everything, might be faster then
        // using the pattern
        database.files.get_file_entries(file_entries);
      }
      else
      {
        database.files.get_file_entries(*i, file_entries);
      }
    }

    for(std::vector<FileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
    {
      ImagePtr image = Image::create(i->get_url(), DatabaseTileProvider::create(*i));
      workspace.add_image(image);
      
      TileEntry tile_entry;
      if (database.tiles.get_tile(*i, i->get_thumbnail_scale(), Vector2i(0,0), tile_entry))
      {
        image->receive_tile(*i, Tile(tile_entry));
      }
       
      // print progress
      int n = (i - file_entries.begin())+1;
      int total = file_entries.size();
      std::cout << "Getting tiles: " << n << "/" << total << " - "
                << (100 * n / total) << '%'
                << '\r' << std::flush;
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
        workspace.add_image(Image::create(*i, TileProviderPtr(new MandelbrotTileProvider(job_manager))));
      }
      else
      {
        std::cout << "Galapix::view(): unknown buildin:// requested: " << *i << " ignoring" << std::endl;
      }
    }
    else if (Filesystem::has_extension(i->str(), "ImageProperties.xml"))
    {
      workspace.add_image(Image::create(*i, ZoomifyTileProvider::create(*i, job_manager)));
    }
    else
    {
      //database_thread.request_file(*i, boost::bind(&Workspace::receive_file, &workspace, _1));
      FileEntry file_entry = database.files.get_file_entry(*i);
      if (!file_entry)
      {
        workspace.add_image(Image::create(*i));
      }
      else
      {
        ImagePtr image = Image::create(file_entry.get_url(), DatabaseTileProvider::create(file_entry));
        workspace.add_image(image);

        TileEntry tile_entry;
        if (database.tiles.get_tile(file_entry, file_entry.get_thumbnail_scale(), Vector2i(0,0), tile_entry))
        {
          image->receive_tile(file_entry, Tile(tile_entry));
        }
      }
    }
  }

  if (!urls.empty())
  {
    std::cout << std::endl;
  }

  job_manager.start_thread();  
  database_thread.start_thread();

#ifdef GALAPIX_SDL
  Viewer viewer(&workspace);
  SDLViewer sdl_viewer(geometry, fullscreen, anti_aliasing, viewer);
  viewer.layout_tight();
  viewer.finish_layout();
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
    Options opts;
    opts.threads  = 2;
    opts.database = Filesystem::get_home() + "/.galapix/cache2.sqlite";
    parse_args(argc, argv, opts);

    if (curl_global_init(CURL_GLOBAL_ALL) != 0)
    {
      std::cout << "Galapix::main(): curl_global_init() failed" << std::endl;
      return EXIT_FAILURE;
    }

    run(opts);

    curl_global_cleanup();

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

  std::vector<URL> urls;

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
      if (urls.empty() && opts.patterns.empty())
      {
        std::cout << "Galapix::run(): Error: No URLs given" << std::endl;
      }
      else
      {
        view(opts, urls);
      }
    }
    else if (command == "check")
    {
      check(opts.database);
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
      filegen(opts, urls);
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
          opts.patterns.push_back(argv[i]);
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
