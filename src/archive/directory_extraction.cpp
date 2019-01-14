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

#include "archive/directory_extraction.hpp"

#include <logmich/log.hpp>

#include "util/blob.hpp"

namespace {

void
collect_files(const boost::filesystem::path& path,
              const boost::filesystem::path& base,
              std::vector<std::string>& lst)
{
  try
  {
    for(boost::filesystem::directory_iterator it(path);
        it != boost::filesystem::directory_iterator();
        ++it)
    {
      if (is_directory(it->symlink_status()))
      {
        collect_files(it->path(), base / it->path().filename(), lst);
      }
      else if (is_regular_file(it->symlink_status()))
      {
        lst.push_back((base / it->path().filename()).string());
      }
      else
      {
        // ignore symlink, device files, etc.
      }
    }
  }
  catch(const std::exception& err)
  {
    log_error(err.what());
  }
}

} // namespace

DirectoryExtraction::DirectoryExtraction(const std::string& path,
                                         const std::string& type) :
  m_path(path),
  m_type(type)
{
}

std::vector<std::string>
DirectoryExtraction::get_filenames() const
{
  std::vector<std::string> lst;

  collect_files(m_path, boost::filesystem::path(), lst);

  return lst;
}

Blob
DirectoryExtraction::get_file(const std::string& filename) const
{
  return Blob::from_file((m_path / filename).string());
}

std::string
DirectoryExtraction::get_file_as_path(const std::string& filename) const
{
  return (m_path / filename).string();
}

/* EOF */
