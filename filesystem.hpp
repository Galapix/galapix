/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#ifndef HEADER_FILESYSTEM_HPP
#define HEADER_FILESYSTEM_HPP

#include <string>
#include <vector>

class Filesystem
{
private:
  static std::string home_directory;

public:
  static bool is_directory(const std::string& pathname);
  static bool exist(const std::string& pathname);
  static void mkdir(const std::string& pathname);
  static std::vector<std::string> open_directory(const std::string& pathname);
  static std::string getxattr(const std::string& pathname);
  static std::string get_home() { return home_directory; }
  static std::string realpath(const std::string& pathname);
  static bool has_extension(const std::string& pathname, const std::string& ext);
  static void copy_mtime(const std::string& from_filename, const std::string& to_filename);
  static unsigned int get_mtime(const std::string& filename);
  
  /** Generate a recursive list of all JPEGs in pathname */
  static void generate_jpeg_file_list(const std::string& pathname, std::vector<std::string>& file_list);

  static void init();
  static void deinit();
};

#endif

/* EOF */
