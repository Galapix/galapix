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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_URL_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_URL_HPP

#include <string>

class ResourceURL
{
public:
  /** scheme ":" [ "//" authority ] path */
  static ResourceURL from_string(const std::string& url);

public:
  ResourceURL() : m_scheme(), m_authority(), m_path() {}

  ResourceURL(const std::string& scheme,
              const std::string& authority,
              const std::string& path);

  ResourceURL(const std::string& scheme,
              const std::string& path);

  /** Usually "http" or "file" */
  std::string get_scheme() const { return m_scheme; }

  /** [ userinfo "@" ] host [ ":" port ] */
  std::string get_authority() const { return m_authority; }
  std::string get_path() const { return m_path; }

  bool is_remote() const;

  std::string str() const;

  bool operator==(const ResourceURL& other) const;
  bool operator!=(const ResourceURL& other) const;
  bool operator<(const ResourceURL& other) const;

private:
  std::string m_scheme;
  std::string m_authority;
  std::string m_path;
};

#endif

/* EOF */
