/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#include "resource/resource_url.hpp"

#include <stdexcept>
#include <sstream>

#include "util/filesystem.hpp"
#include "util/format.hpp"

ResourceURL::ResourceURL(const std::string& scheme,
                         const std::string& authority,
                         const std::string& path) :
  m_scheme(scheme),
  m_authority(authority),
  m_path(path)
{
  if (m_scheme == "file")
  {
    m_path = Filesystem::realpath(m_path);
  }
}

ResourceURL::ResourceURL(const std::string& scheme,
                         const std::string& path) :
  m_scheme(scheme),
  m_authority(),
  m_path(path)
{
  if (m_scheme == "file")
  {
    m_path = Filesystem::realpath(m_path);
  }
}

ResourceURL
ResourceURL::from_string(const std::string& url)
{
  std::string::size_type scheme_pos = url.find(':');
  if (scheme_pos == std::string::npos)
  {
    throw std::runtime_error(format("not a valid url: %s", url));
  }
  else
  {
    if (scheme_pos + 2 < url.size() &&
        url[scheme_pos+1] == '/' &&
        url[scheme_pos+2] == '/')
    {
      std::string::size_type domain_start = scheme_pos + 3;
      std::string::size_type domain_end = url.find('/', domain_start);
      if (domain_end == std::string::npos)
      {
        domain_end = url.size();
      }
      return ResourceURL(url.substr(0, scheme_pos),
                         url.substr(domain_start, domain_end - domain_start),
                         url.substr(domain_end));
    }
    else
    {
      // assume URL lacks the // and thus the domain part, e.g.:
      // file:/home/juser/foobar.txt
      return ResourceURL(url.substr(0, scheme_pos),
                         url.substr(scheme_pos+1));
    }
  }
}

std::string 
ResourceURL::str() const
{
  std::ostringstream out;
  
  out << m_scheme << "://";
  if (!m_authority.empty())
  {
    out << m_authority;
  }
  out << m_path;
  
  return out.str();
}

/* EOF */
