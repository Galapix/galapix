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

#ifndef HEADER_GALAPIX_GALAPIX_GALAPIX_HPP
#define HEADER_GALAPIX_GALAPIX_GALAPIX_HPP

#include <vector>
#include <string>

#include "util/url.hpp"
#include "math/size.hpp"

class Options;

class Galapix
{
private:
  bool fullscreen;
  Size geometry;
  int  anti_aliasing;

public:
  Galapix();
  ~Galapix();

  void print_usage();
  void parse_args(int argc, char** argv, Options& options);
  void run(const Options& opts);
  int  main(int argc, char** argv);

  void merge(const std::string& database, const std::vector<std::string>& filenames);
  void test(const Options& opts, const std::vector<URL>& filenames);
  void info(const std::vector<URL>& filenames);
  void downscale(const std::vector<URL>& filenames);
  void cleanup(const std::string& database);
  void check(const std::string& database);
  void list(const Options& opts);
  void thumbgen(const Options& opts,
                const std::vector<URL>& filenames, 
                bool generate_all_tiles);
  void filegen(const Options& opts,
               const std::vector<URL>& filenames);
  void export_images(const std::string& database, const std::vector<URL>& url);
  void view(const Options& opts,
            const std::vector<URL>& filenames,
            bool view_all);
};

#endif

/* EOF */
