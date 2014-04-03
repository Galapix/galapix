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

#ifndef HEADER_GALAPIX_UTIL_FILESYSTEM_HPP
#define HEADER_GALAPIX_UTIL_FILESYSTEM_HPP

#include "util/url.hpp"

class Filesystem
{
private:
  static std::string home_directory;

public:
  static std::string find_exe(const std::string& name);

  static bool is_directory(const std::string& pathname);
  static bool exist(const std::string& pathname);
  static void mkdir(const std::string& pathname);
  static void remove(const std::string& filename);
  static std::vector<std::string> open_directory(const std::string& pathname);

  static void open_directory_recursivly(const std::string& pathname, std::vector<std::string>& lst);

  static void readlines_from_file(const std::string& pathname, std::vector<std::string>& lst);

  static std::string getxattr(const std::string& pathname);
  static std::string get_home() { return home_directory; }

  static std::string realpath_system(const std::string& pathname);
  static std::string realpath_fast(const std::string& pathname);
  static std::string realpath(const std::string& pathname);

  static std::string get_extension(const std::string& pathname);
  static bool has_extension(const std::string& pathname, const std::string& ext);
  static void copy_mtime(const std::string& from_filename, const std::string& to_filename);

  /** read the first 512 bytes of the archive for magic detection */
  static std::string  get_magic(const std::string& filename);

  static unsigned int get_mtime(const std::string& filename);
  static unsigned int get_size(const std::string& filename);

  /** Generate a recursive list of all images in pathname */
  static void generate_image_file_list(const std::string& pathname, std::vector<URL>& file_list);

  static void init();
  static void deinit();
};

#endif

/* EOF */
