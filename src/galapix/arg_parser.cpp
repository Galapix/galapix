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

#include "galapix/arg_parser.hpp"

#include <iostream>
#include <fstream>
#include <string.h>
#include <logmich/log.hpp>

#include "galapix/options.hpp"
#include "util/raise_exception.hpp"

namespace galapix {

void
ArgParser::print_usage()
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

void
ArgParser::print_version()
{
  std::cout << PROJECT_NAME << " " << PROJECT_VERSION << std::endl;
}

void
ArgParser::parse_args(int argc, char** argv, Options& opts)
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
      else if (strcmp(argv[i], "--version") == 0 ||
               strcmp(argv[i], "-V") == 0)
      {
        print_version();
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
        logmich::set_log_level(logmich::LogLevel::DEBUG);
      }
      else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0)
      {
        logmich::set_log_level(logmich::LogLevel::INFO);
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
        if (i < argc) {
          opts.patterns.push_back(argv[i]);
        } else {
          raise_runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
        }
      }
      else if (strcmp(argv[i], "--anti-aliasing") == 0 ||
               strcmp(argv[i], "-a") == 0)
      {
        i += 1;
        if (i < argc) {
          opts.anti_aliasing = atoi(argv[i]);
        } else {
          raise_runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
        }
      }
      else if (strcmp(argv[i], "--geometry") == 0 ||
               strcmp(argv[i], "-g") == 0)
      {
        i += 1;
        if (i < argc) {
          int width = 0;
          int height = 0;
          sscanf(argv[i], "%dx%d", &width, &height);
          opts.geometry = Size(width, height);
        } else {
          raise_runtime_error(std::string("Option ") + argv[i-1] + " requires an argument");
        }
      }
      else if (strcmp(argv[i], "--fullscreen") == 0 ||
               strcmp(argv[i], "-f") == 0)
      {
        opts.fullscreen = true;
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

} // namespace galapix

/* EOF */
