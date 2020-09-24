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

#include "archive/rar_archive_loader.hpp"

#include "archive/archive_manager.hpp"
#include "archive/incremental_extraction.hpp"
#include "archive/rar.hpp"

RarArchiveLoader::RarArchiveLoader()
{
}

std::vector<std::string>
RarArchiveLoader::get_magics() const
{
  return { std::string("Rar!\x1A\a\0", 7) };
}

std::vector<std::string>
RarArchiveLoader::get_extensions() const
{
  return { "rar", "cbr" };
}

std::vector<std::string>
RarArchiveLoader::get_filenames(const std::string& zip_filename) const
{
  return Rar::get_filenames(zip_filename);
}

std::vector<uint8_t>
RarArchiveLoader::get_file(const std::string& zip_filename, const std::string& filename) const
{
  return Rar::get_file(zip_filename, filename);
}

void
RarArchiveLoader::extract(const std::string& archive, const std::string& target_directory) const
{
  Rar::extract(archive, target_directory);
}

/* EOF */
