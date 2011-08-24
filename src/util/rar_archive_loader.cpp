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

#include "util/rar_archive_loader.hpp"

#include "util/archive_manager.hpp"
#include "plugins/rar.hpp"

RarArchiveLoader::RarArchiveLoader()
{
}

void
RarArchiveLoader::register_loader(ArchiveManager& manager)
{
  manager.register_by_extension(this, "rar");
  manager.register_by_extension(this, "cbr");

  manager.register_by_magic(this, std::string("Rar!\x1A\a\0", 7));
}

std::vector<std::string>
RarArchiveLoader::get_filenames(const std::string& zip_filename)
{
  return Rar::get_filenames(zip_filename);
}

BlobPtr
RarArchiveLoader::get_file(const std::string& zip_filename, const std::string& filename)
{
  return Rar::get_file(zip_filename, filename);
}

/* EOF */
