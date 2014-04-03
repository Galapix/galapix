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

#include "resource/blob_accessor.hpp"
#include "util/blob.hpp"
#include "util/currenton.hpp"

class ArchiveLoader;
class Extraction;

class ArchiveManager : public Currenton<ArchiveManager>
{
private:
  std::string m_tmpdir;
  std::vector<std::unique_ptr<ArchiveLoader> > m_loader;
  std::map<std::string, ArchiveLoader*> m_loader_by_file_exts;
  std::map<std::string, ArchiveLoader*> m_loader_by_magic;

public:
  ArchiveManager();
  ArchiveManager(const std::string& tmpdir);
  ~ArchiveManager();

  bool is_archive(const std::string& filename) const;
  bool is_archive(const BlobPtr& blob) const;

  /**
     Returns the list of files contained in the archive, if \a loader
     is supply the loader used in the process will be returned in
     it.
  */
  std::vector<std::string> get_filenames(const std::string& zip_filename,
                                         const ArchiveLoader** loader_out = nullptr) const;

  BlobPtr get_file(const std::string& zip_filename, const std::string& filename) const;
  BlobAccessorPtr get_file(const BlobAccessorPtr& archive, const std::string& type, const std::string& args) const;

  /**
     Returns an Extraction object that allows fast access to files
     inside the archive. For archives that don't allow seeking this
     means the file are extracted to a temporary directiory, seekable
     archives are accessed directly.
  */
  std::shared_ptr<Extraction> get_extraction(const std::string& filename) const;

private:
  /** Returns a path to an empty directory that can be used for extractions */
  std::string create_extraction_directory() const;

  const ArchiveLoader& get_loader(const std::string& filename) const;
  const ArchiveLoader* find_loader_by_filename(const std::string& filename) const;
  const ArchiveLoader* find_loader_by_magic(const std::string& filename) const;

private:
  ArchiveManager(const ArchiveManager&);
  ArchiveManager& operator=(const ArchiveManager&);
};

#endif

/* EOF */
