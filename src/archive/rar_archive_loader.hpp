/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_UTIL_RAR_ARCHIVE_LOADER_HPP
#define HEADER_GALAPIX_UTIL_RAR_ARCHIVE_LOADER_HPP

#include "archive/archive_loader.hpp"

class RarArchiveLoader : public ArchiveLoader
{
private:
public:
  RarArchiveLoader();

  void register_loader(ArchiveManager& manager);

  std::vector<std::string> get_filenames(const std::string& zip_filename) const;
  BlobPtr get_file(const std::string& zip_filename, const std::string& filename) const;
  std::shared_ptr<Extraction> get_extraction(const std::string& filename) const;

  std::string str() const { return "rar"; }

private:
  RarArchiveLoader(const RarArchiveLoader&);
  RarArchiveLoader& operator=(const RarArchiveLoader&);
};

#endif

/* EOF */
