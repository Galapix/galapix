// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "galapix/galapix.hpp"

#include <algorithm>
#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <vector>

#include <Magick++.h>

#include "archive/archive_manager.hpp"
#include "database/database.hpp"
#include "display/framebuffer.hpp"
#include "display/surface.hpp"
#include "galapix/arg_parser.hpp"
#include "galapix/options.hpp"
#include "galapix/system.hpp"
#include "galapix/thumbnail_generator.hpp"
#include "galapix/viewer.hpp"
#include "galapix/viewer_command.hpp"
#include "galapix/workspace.hpp"
#include "generator/generator.hpp"
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
#include "resource/blob_manager.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_metadata.hpp"
#include "server/database_thread.hpp"
#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"
#include "util/string_util.hpp"

Galapix::Galapix(System& system) :
  m_system(system)
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

      SoftwareSurface target = SoftwareSurface::create(PixelData::RGB_FORMAT, size);
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
  Database db = Database::create(database);
  std::cout << "Running database cleanup routines, this process can take multiple minutes." << std::endl;
  std::cout << "You can interrupt it via Ctrl-c, which won't do harm, but will throw away all the cleanup work done till that point" << std::endl;
  db.cleanup();
  std::cout << "Running database cleanup routines done" << std::endl;
}

void
Galapix::info(const Options& opts)
{
  Database db = Database::create(opts.database);
  JobManager job_manager(4);

  job_manager.start_thread();

  DatabaseThread database(db, job_manager);
  DownloadManager download_mgr;
  BlobManager blob_mgr(download_mgr, ArchiveManager::current());
  Generator generator(blob_mgr, ArchiveManager::current());

  database.start_thread();

  ResourceManager resource_mgr(database, generator, download_mgr,
                               ArchiveManager::current());

  std::atomic_int count = 0;
  auto files = std::vector<std::string>(opts.rest.begin() + 1, opts.rest.end());
  for(const auto& filename : files)
  {
    auto callback = [&count, filename](const Failable<ResourceMetadata>& data)
      {
#if 0
        try
        {
          auto resource_info = data.get();
          std::cout << filename << ":\n";

          if (resource_info.get_source_type() == SourceType::File)
          {
            auto file_info = resource_info.get_file_info();
            std::cout
              << "  rowid: " << file_info.get_id() << "\n"
              << "   path: " << file_info.get_path() << "\n"
              << "   sha1: " << file_info.get_blob_info().get_sha1() << "\n"
              << "   size: " << file_info.get_blob_info().get_size() << "\n"
              << "  mtime: " << file_info.get_mtime() << "\n"
              << std::endl;
          }
          else if (resource_info.get_source_type() == SourceType::URL)
          {
            std::cout << "--URL--" << std::endl;
          }
        }
        catch(const std::exception& err)
        {
          std::cout << "error: " << err.what() << std::endl;
        }
#endif
        count -= 1;
      };

    ResourceLocator locator = ResourceLocator::from_string(filename);
    resource_mgr.request_resource_metadata(locator, callback);

    count += 1;
  }

  log_debug("going into loop");
  while(count > 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  log_debug("going out of loop");

  database.abort_thread();
  database.join_thread();

  job_manager.stop_thread();
  job_manager.join_thread();
}

void
Galapix::list(const Options& opts)
{
  Database db = Database::create(opts.database);

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

namespace {

void print_exception(std::exception const& err, int level = 0)
{
  std::cout << std::string(level * 2, ' ') << "exception: " << err.what() << std::endl;
  try
  {
    std::rethrow_if_nested(err);
  }
  catch(std::exception const& nested_err)
  {
    print_exception(nested_err, level + 1);
  }
  catch(...)
  {
    std::cout << "unknown exception\n";
  }
}

} // namespace

int
Galapix::main(int argc, char** argv)
{
  try
  {
    Options opts;
    opts.threads  = 2;
    opts.database = Filesystem::get_home() + "/.galapix/cache4";
    ArgParser::parse_args(argc, argv, opts);

    if (!sqlite3_threadsafe())
    {
      raise_runtime_error("Error: SQLite must be compiled with SQLITE_THREADSAFE");
    }

    Magick::InitializeMagick(*argv);

    DownloadManager download_manager;
    ArchiveManager archive_manager;
    SoftwareSurfaceFactory software_surface_factory;

    run(opts);

    return EXIT_SUCCESS;
  }
  catch(const std::exception& err)
  {
    std::cout << "Exception:\n";
    print_exception(err);
    return EXIT_FAILURE;
  }
}

void
Galapix::run(const Options& opts)
{
  std::cout << "Using database: " << (opts.database.empty() ? "memory" : opts.database) << std::endl;

  if (opts.rest.empty())
  {
    if (m_system.requires_command_line_args())
    {
      ArgParser::print_usage();
    }
    else
    {
      ViewerCommand viewer(m_system, opts);
      viewer.run(std::vector<URL>());
    }
  }
  else
  {
    auto generate_urls = [&opts]() -> std::vector<URL>
      {
        std::vector<URL> urls;

        std::cout << "Scanning directories... " << std::flush;
        for(std::vector<std::string>::const_iterator i = opts.rest.begin()+1; i != opts.rest.end(); ++i)
        {
          if (URL::is_url(*i)) {
            urls.push_back(URL::from_string(*i));
          } else {
            Filesystem::generate_image_file_list(*i, urls);
          }
        }
        std::sort(urls.begin(), urls.end(),
                  [](const URL& lhs, const URL& rhs) {
                    return StringUtil::numeric_less(lhs.str(), rhs.str());
                  });
        std::cout << urls.size() << " files found." << std::endl;
        return urls;
      };

    const std::string& command = opts.rest.front();

    if (command == "view")
    {
      auto urls = generate_urls();
      if (urls.empty() && opts.patterns.empty())
      {
        std::cout << "Galapix::run(): Error: No URLs given" << std::endl;
      }
      else
      {
        ViewerCommand viewer(m_system, opts);
        viewer.run(urls);
      }
    }
    else if (command == "info")
    {
      info(opts);
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
      auto urls = generate_urls();
      export_images(opts.database, urls);
    }
    else if (command == "thumbgen")
    {
      auto urls = generate_urls();
      ThumbnailGenerator generator(opts);
      generator.run(urls, false);
    }
    else
    {
      ArgParser::print_usage();
    }
  }
}

/* EOF */
