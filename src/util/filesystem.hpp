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

#ifndef HEADER_GALAPIX_UTIL_FILESYSTEM_HPP
#define HEADER_GALAPIX_UTIL_FILESYSTEM_HPP

#include "util/url.hpp"

class Filesystem
{
private:
  static std::string home_directory;

public:
  static std::string find_exe(std::string const& name);

  static bool is_directory(std::string const& pathname);
  static bool exist(std::string const& pathname);
  static void mkdir(std::string const& pathname);
  static void remove(std::string const& filename);
  static std::vector<std::string> open_directory(std::string const& pathname);

  static void open_directory_recursivly(std::string const& pathname, std::vector<std::string>& lst);

  static void readlines_from_file(std::string const& pathname, std::vector<std::string>& lst);

  static std::string getxattr(std::string const& pathname);
  static std::string get_home() { return home_directory; }

  static std::string realpath_system(std::string const& pathname);
  static std::string realpath_fast(std::string const& pathname);
  static std::string realpath(std::string const& pathname);

  static std::string get_extension(std::string const& pathname);
  static bool has_extension(std::string const& filename, std::string const& ext);
  static void copy_mtime(std::string const& from_filename, std::string const& to_filename);

  /** read the first 512 bytes of the archive for magic detection */
  static std::string  get_magic(std::string const& filename);

  static time_t get_mtime(std::string const& filename);
  static size_t get_size(std::string const& filename);

  /** Generate a recursive list of all images in pathname */
  static void generate_image_file_list(std::string const& pathname, std::vector<URL>& file_list);

  static void init();
  static void deinit();
};

#endif

/* EOF */
