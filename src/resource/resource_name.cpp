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

#include "resource/resource_name.hpp"

ResourceName::ResourceName() :
  m_id(),
  m_blob_info(),
  m_handler()
{}

ResourceName::ResourceName(const BlobInfo& blob_info,
                           const ResourceHandler& handler) :
  m_id(),
  m_blob_info(blob_info),
  m_handler(handler)
{}

std::string
ResourceName::str() const
{
  std::ostringstream os;
  os << "sha1:" << m_blob_info.get_sha1().str() << "-" << m_blob_info.get_size() << "//" << m_handler.str();
  return os.str();
}

/* EOF */
