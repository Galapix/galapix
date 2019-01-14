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

#include "resource/blob_accessor.hpp"

#include <boost/filesystem.hpp>

BlobAccessor::BlobAccessor(const std::string& filename) :
  m_mutex(),
  m_filename(filename),
  m_blob(),
  m_blob_info()
{
}

BlobAccessor::BlobAccessor(Blob const& blob) :
  m_mutex(),
  m_filename(),
  m_blob(blob),
  m_blob_info()
{
}

bool
BlobAccessor::has_stdio_name() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return !m_filename.empty();
}

std::string
BlobAccessor::get_stdio_name() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_filename.empty())
  {
    return m_filename;
  }
  else
  {
    boost::filesystem::path tmpfile = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("%%%%-%%%%-%%%%-%%%%");
    m_filename = tmpfile.string();
    m_blob.write_to_file(m_filename);
    return m_filename;
  }
}

bool
BlobAccessor::has_blob() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  return static_cast<bool>(m_blob);
}

Blob
BlobAccessor::get_blob() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_blob)
  {
    return m_blob;
  }
  else
  {
    m_blob = Blob::from_file(m_filename);
    return m_blob;
  }
}

BlobInfo
BlobAccessor::get_blob_info() const
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if (m_blob_info)
  {
    return *m_blob_info;
  }
  else
  {
    if (!m_filename.empty())
    {
      m_blob_info = BlobInfo(SHA1::from_file(m_filename), Filesystem::get_size(m_filename));
    }
    else
    {
      m_blob_info = BlobInfo(SHA1::from_mem(m_blob.get_data(), m_blob.size()),
                             m_blob.size());
    }
    return *m_blob_info;
  }
}

size_t
BlobAccessor::size() const
{
  // TODO: rewrite this to handle stdio
  return get_blob().size();
}

const uint8_t*
BlobAccessor::get_data() const
{
  // TODO: rewrite this to handle stdio
  return get_blob().get_data();
}

/* EOF */
