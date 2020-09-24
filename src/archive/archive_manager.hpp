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

#ifndef HEADER_GALAPIX_ARCHIVE_ARCHIVE_MANAGER_HPP
#define HEADER_GALAPIX_ARCHIVE_ARCHIVE_MANAGER_HPP

#include <filesystem>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <vector>

class ArchiveLoader;
class Extraction;

class ArchiveManager
{
public:
  ArchiveManager();
  ArchiveManager(const std::string& tmpdir);
  ~ArchiveManager();

  bool is_archive(const std::string& filename) const;
  bool is_archive(std::span<uint8_t const> data) const;

  /** Returns the list of files contained in the archive, if \a loader
      is supply the loader used in the process will be returned in
      it. */
  std::vector<std::string> get_filenames(const std::string& zip_filename,
                                         const ArchiveLoader** loader_out = nullptr) const;

  std::vector<uint8_t> get_file(const std::string& archive_filename, const std::string& filename) const;
  std::vector<uint8_t> get_file(std::string const& archive_filename, const std::string& type, const std::string& filename) const;

  /** Returns an Extraction object that allows fast access to files
      inside the archive. For archives that don't allow seeking this
      means the file are extracted to a temporary directiory, seekable
      archives are accessed directly. */
  std::shared_ptr<Extraction> get_extraction(const std::string& archive_filename) const;

private:
  /** Returns a path to an empty directory that can be used for extractions */
  std::string create_extraction_directory() const;

  void add_loader(std::unique_ptr<ArchiveLoader> loader);

  const ArchiveLoader& get_loader(const std::string& filename) const;
  const ArchiveLoader* find_loader_by_filename(const std::string& filename) const;
  const ArchiveLoader* find_loader_by_magic(const std::string& filename) const;

private:
  std::filesystem::path m_tmpdir;
  std::vector<std::unique_ptr<ArchiveLoader> > m_loader;
  std::map<std::string, ArchiveLoader*> m_loader_by_file_exts;
  std::map<std::string, ArchiveLoader*> m_loader_by_magic;

private:
  ArchiveManager(const ArchiveManager&);
  ArchiveManager& operator=(const ArchiveManager&);
};

#endif

/* EOF */
