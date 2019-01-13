/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_ARCHIVE_ZIP_ARCHIVE_LOADER_HPP
#define HEADER_GALAPIX_ARCHIVE_ZIP_ARCHIVE_LOADER_HPP

#include "archive/archive_loader.hpp"

class ZipArchiveLoader final : public ArchiveLoader
{
public:
  ZipArchiveLoader();

  std::vector<std::string> get_magics() const override;
  std::vector<std::string> get_extensions() const override;

  std::vector<std::string> get_filenames(const std::string& zip_filename) const override;
  Blob get_file(const std::string& zip_filename, const std::string& filename) const override;
  void extract(const std::string& archive, const std::string& target_directory) const override;
  bool is_seekable(const std::string& archive) const override { return true; }

  std::string str() const override { return "zip"; }

private:
  ZipArchiveLoader(const ZipArchiveLoader&);
  ZipArchiveLoader& operator=(const ZipArchiveLoader&);
};

#endif

/* EOF */
