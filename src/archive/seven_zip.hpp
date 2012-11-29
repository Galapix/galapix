/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_PLUGINS_SEVEN_ZIP_HPP
#define HEADER_GALAPIX_PLUGINS_SEVEN_ZIP_HPP

#include <string>

#include "util/blob.hpp"

/** Support for 7-Zip archives */
class SevenZip
{
public:
  static std::vector<std::string> get_filenames(const std::string& zip_filename);

  /** FIXME: Not very practical as 7-Zip has to extract the whole
      archive to get to a single file */
  static BlobPtr get_file(const std::string& zip_filename, const std::string& filename);

  /**
     Extract the content of \a archive to \a target_directory, \a
     target_directory will be created if it does not exist prior to
     calling this function. \a target_directory must be empty if it
     already exists.
  */
  static void extract(const std::string& archive, const std::string& target_directory);
};

#endif

/* EOF */
