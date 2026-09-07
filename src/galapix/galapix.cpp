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
#include <iostream>
#include <string>
#include <vector>

#include <sqlite3.h>
#include <Magick++.h>

#include <arxp/archive_manager.hpp>
#include <strut/numeric_less.hpp>
#include <surf/software_surface_factory.hpp>

#include "galapix/app.hpp"
#include "galapix/arg_parser.hpp"
#include "galapix/options.hpp"
#include "galapix/system.hpp"
#include "galapix/viewer_command.hpp"
#include "network/download_manager.hpp"
#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"
#include "util/url.hpp"

namespace galapix {

using namespace surf;

Galapix::Galapix(System& system) :
  m_system(system)
{
  Filesystem::init();
}

Galapix::~Galapix()
{
  Filesystem::deinit();
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
    arxp::ArchiveManager archive_manager;
    SoftwareSurfaceFactory software_surface_factory;

    g_app.m_archive_manager = &archive_manager;
    g_app.m_surface_factory = &software_surface_factory;

    run(opts);

    return EXIT_SUCCESS;
  }
  catch(std::exception const& err)
  {
    std::cout << "Exception:\n";
    print_exception(err);
    return EXIT_FAILURE;
  }
}

void
Galapix::run(Options const& opts)
{
  std::cout << "Using database: " << (opts.database.empty() ? "memory" : opts.database) << std::endl;

  // Subcommands (view / thumbgen / list / cleanup / …) are gone. Non-option
  // arguments are files or URLs. A leading "view" is still accepted for
  // backwards compatibility with old scripts and docs.
  std::vector<std::string> paths = opts.rest;
  if (!paths.empty())
  {
    std::string const& first = paths.front();
    if (first == "view")
    {
      paths.erase(paths.begin());
    }
    else if (first == "thumbgen" || first == "list" || first == "cleanup" ||
             first == "info" || first == "export")
    {
      raise_runtime_error(
        "Command '" + first + "' is no longer supported. "
        "Thumbnail generation and database maintenance belong to thumtoo "
        "(thumtoo-prepare / thumtoo CLI). Galapix is the viewer only; "
        "pass image files or URLs directly.");
    }
  }

  if (paths.empty() && opts.patterns.empty())
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
    return;
  }

  std::vector<URL> urls;
  std::cout << "Scanning directories... " << std::flush;
  for (std::string const& path : paths)
  {
    if (URL::is_url(path)) {
      urls.push_back(URL::from_string(path));
    } else {
      Filesystem::generate_image_file_list(path, urls);
    }
  }
  std::sort(urls.begin(), urls.end(),
            [](URL const& lhs, URL const& rhs) {
              return strut::numeric_less(lhs.str(), rhs.str());
            });
  std::cout << urls.size() << " files found." << std::endl;

  if (urls.empty() && opts.patterns.empty())
  {
    std::cout << "Galapix::run(): Error: No URLs given" << std::endl;
    return;
  }

  ViewerCommand viewer(m_system, opts);
  viewer.run(urls);
}

} // namespace galapix

/* EOF */
