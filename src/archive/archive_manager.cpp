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

#include "archive/archive_manager.hpp"

#include <string.h>
#include <stdexcept>
#include <boost/filesystem.hpp>

#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"

#include "archive/directory_extraction.hpp"
#include "archive/incremental_extraction.hpp"
#include "archive/rar_archive_loader.hpp"
#include "archive/seven_zip_archive_loader.hpp"
#include "archive/tar_archive_loader.hpp"
#include "archive/zip_archive_loader.hpp"
#include "util/filesystem.hpp"

namespace {
bool has_prefix(const std::string& lhs, const std::string& rhs)
{
  if (lhs.length() < rhs.length())
    return false;
  else
    return lhs.compare(0, rhs.length(), rhs) == 0;
}
} // namespace

ArchiveManager::ArchiveManager() :
  ArchiveManager(boost::filesystem::temp_directory_path().string())
{
}

ArchiveManager::ArchiveManager(const std::string& tmpdir) :
  m_tmpdir(tmpdir),
  m_loader(),
  m_loader_by_file_exts(),
  m_loader_by_magic()
{
  m_loader.push_back(std::unique_ptr<ArchiveLoader>(new RarArchiveLoader));
  m_loader.push_back(std::unique_ptr<ArchiveLoader>(new ZipArchiveLoader));
  m_loader.push_back(std::unique_ptr<ArchiveLoader>(new TarArchiveLoader));
  m_loader.push_back(std::unique_ptr<ArchiveLoader>(new SevenZipArchiveLoader));

  for(auto& loader: m_loader)
  {
    for(const auto& magic: loader->get_magics())
    {
      m_loader_by_magic[magic] = loader.get();
    }

    for(const auto& ext: loader->get_extensions())
    {
      m_loader_by_file_exts[ext] = loader.get();
    }
  }
}

ArchiveManager::~ArchiveManager()
{
}

bool
ArchiveManager::is_archive(const std::string& filename) const
{
  try
  {
    get_loader(filename);
    return true;
  }
  catch(...)
  {
    return false;
  }
}

bool
ArchiveManager::is_archive(const BlobPtr& blob) const
{
  log_error("not implemented");
  return false;
}

const ArchiveLoader& 
ArchiveManager::get_loader(const std::string& filename) const
{
  auto loader = find_loader_by_magic(filename);

  if (!loader)
  {
    loader = find_loader_by_filename(filename);
  }

  if (!loader)
  {
    raise_exception(std::runtime_error, "failed to find loader for archive file: " << filename);
  }
  else
  {
    return *loader;
  }
}

const ArchiveLoader*
ArchiveManager::find_loader_by_filename(const std::string& filename) const
{
  for(const auto& ext: m_loader_by_file_exts)
  {
    if (Filesystem::has_extension(filename, ext.first))
    {
      return ext.second;
    }
  }
  return nullptr;
}

const ArchiveLoader*
ArchiveManager::find_loader_by_magic(const std::string& filename) const
{
  std::string start_of_file = Filesystem::get_magic(filename);

  // search for a loader that can handle the magic
  for(const auto& it: m_loader_by_magic)
  {
    if (has_prefix(start_of_file, it.first))
    {
      return it.second;
    }
  }
  
  return nullptr;
}

std::vector<std::string>
ArchiveManager::get_filenames(const std::string& zip_filename, 
                              const ArchiveLoader** loader_out) const
{
  const auto& loader = get_loader(zip_filename);
  const auto& files = loader.get_filenames(zip_filename);
  if (loader_out) 
  {
    *loader_out = &loader;
  }
  return files;
}

BlobPtr
ArchiveManager::get_file(const std::string& archive, const std::string& filename) const
{
  const auto& loader = get_loader(archive);
  return loader.get_file(archive, filename);
}

std::shared_ptr<Extraction>
ArchiveManager::get_extraction(const std::string& archive) const
{
  const auto& loader = get_loader(archive);
  if (loader.is_seekable(archive))
  {
    return std::make_shared<IncrementalExtraction>(loader, archive);
  }
  else
  {
    std::string tmpdir = create_extraction_directory();
    loader.extract(archive, tmpdir);
    return std::make_shared<DirectoryExtraction>(tmpdir, loader.str());
  }
}

std::string
ArchiveManager::create_extraction_directory() const
{
  boost::filesystem::path directory = m_tmpdir / boost::filesystem::unique_path("%%%%-%%%%-%%%%-%%%%");
  log_info("creating directory: " << directory);
  boost::filesystem::create_directory(directory);
  return directory.string();
}

/* EOF */
