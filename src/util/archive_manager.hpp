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

#ifndef HEADER_GALAPIX_UTIL_ARCHIVE_MANAGER_HPP
#define HEADER_GALAPIX_UTIL_ARCHIVE_MANAGER_HPP

#include <memory>
#include <map>
#include <string>
#include <vector>

#include "util/blob.hpp"
#include "util/currenton.hpp"

class ArchiveLoader;

class ArchiveManager : public Currenton<ArchiveManager>
{
private:
  std::vector<std::unique_ptr<ArchiveLoader> > m_loader;
  std::map<std::string, ArchiveLoader*> m_loader_by_file_exts;
  std::map<std::string, ArchiveLoader*> m_loader_by_magic;

public:
  ArchiveManager();
  ~ArchiveManager();

  void register_by_magic(ArchiveLoader* loader, const std::string& magic);
  void register_by_extension(ArchiveLoader* loader, const std::string& extension);

  bool is_archive(const std::string& filename) const;

  ArchiveLoader* find_loader_by_filename(const std::string& filename) const;
  ArchiveLoader* find_loader_by_magic(const std::string& filename) const;

  /** Returns the list of files contained in the archive, if \a loader
      is supply the loader used in the process will be returned in
      it */
  std::vector<std::string> get_filenames(const std::string& zip_filename, 
                                         ArchiveLoader** loader_out = nullptr) const;
  BlobPtr get_file(const std::string& zip_filename, const std::string& filename) const;

private:
  ArchiveManager(const ArchiveManager&);
  ArchiveManager& operator=(const ArchiveManager&);
};

#endif

/* EOF */
