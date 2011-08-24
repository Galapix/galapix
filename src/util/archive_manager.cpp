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

#include "util/archive_manager.hpp"

#include <string.h>
#include <stdexcept>
#include <fstream>

#include "util/raise_exception.hpp"

#include "util/rar_archive_loader.hpp"
#include "util/seven_zip_archive_loader.hpp"
#include "util/tar_archive_loader.hpp"
#include "util/zip_archive_loader.hpp"
#include "util/filesystem.hpp"

namespace {
bool has_prefix(const std::string& lhs, const std::string rhs)
{
  if (lhs.length() < rhs.length())
    return false;
  else
    return lhs.compare(0, rhs.length(), rhs) == 0;
}
} // namespace

ArchiveManager::ArchiveManager() :
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
    loader->register_loader(*this);
  }
}

ArchiveManager::~ArchiveManager()
{
}

void
ArchiveManager::register_by_magic(ArchiveLoader* loader, const std::string& magic)
{
  m_loader_by_magic[magic] = loader;
}

void
ArchiveManager::register_by_extension(ArchiveLoader* loader, const std::string& extension)
{
  m_loader_by_file_exts[extension] = loader;
}

bool
ArchiveManager::is_archive(const std::string& filename) const
{
  return find_loader_by_filename(filename) != 0;
}

ArchiveLoader*
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

ArchiveLoader*
ArchiveManager::find_loader_by_magic(const std::string& filename) const
{
  std::string start_of_file;

  { // read the first 512 bytes of the archive for magic detection
    char buf[512];
    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
      raise_exception(std::runtime_error, filename << ": " << strerror(errno));
    }
    else
    {
      if (!in.read(buf, sizeof(buf)))
      {
        raise_exception(std::runtime_error, filename << ": " << strerror(errno));
      }
      else
      {
        start_of_file.assign(buf, in.gcount());
      }
    }
  }

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
ArchiveManager::get_filenames(const std::string& zip_filename) const
{
  auto loader = find_loader_by_filename(zip_filename);
  if (!loader)
  {
    raise_exception(std::runtime_error, "failed to find loader for archive file: " << zip_filename);
  }
  else
  {
    try
    {
      return loader->get_filenames(zip_filename);
    }
    catch(const std::exception& err)
    {
      log_warning << err.what() << std::endl;

      loader = find_loader_by_magic(zip_filename);
      if (!loader)
      {
        raise_exception(std::runtime_error, "failed to find loader for archive file: " << zip_filename);
      }
      else
      {
        return loader->get_filenames(zip_filename);
      }
    }
  }
}

BlobPtr
ArchiveManager::get_file(const std::string& zip_filename, const std::string& filename) const
{
  auto loader = find_loader_by_filename(zip_filename);
  if (!loader)
  {
    raise_exception(std::runtime_error, "failed to find loader for archive file: " << zip_filename);
  }
  else
  {
    try
    {
      return loader->get_file(zip_filename, filename);
    }
    catch(const std::exception& err)
    {
      log_warning << err.what() << std::endl;

      loader = find_loader_by_magic(zip_filename);
      if (!loader)
      {
        raise_exception(std::runtime_error, "failed to find loader for archive file: " << zip_filename);
      }
      else
      {
        return loader->get_file(zip_filename, filename);
      }
    }
  }
}

/* EOF */
