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

#include "zip_archive_loader.hpp"

#include "archive/archive_manager.hpp"
#include "archive/incremental_extraction.hpp"
#include "archive/zip.hpp"

ZipArchiveLoader::ZipArchiveLoader()
{
}

std::vector<std::string>
ZipArchiveLoader::get_magics() const
{
  return {
    "PK\x03\x04", // regular archive
    "PK\x05\x06", // empty archive
    "PK\x07\x08"  // spanned archive
  };
}

std::vector<std::string>
ZipArchiveLoader::get_extensions() const
{
  return { "zip", "cbz" };
}

std::vector<std::string>
ZipArchiveLoader::get_filenames(const std::string& zip_filename) const
{
  return Zip::get_filenames(zip_filename);
}

std::vector<uint8_t>
ZipArchiveLoader::get_file(const std::string& zip_filename, const std::string& filename) const
{
  return Zip::get_file(zip_filename, filename);
}

void
ZipArchiveLoader::extract(const std::string& archive, const std::string& target_directory) const
{
  Zip::extract(archive, target_directory);
}

/* EOF */
