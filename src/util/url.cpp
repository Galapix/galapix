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

#include "util/url.hpp"

#include <assert.h>
#include <stdexcept>
#include <ostream>

#include "archive/rar.hpp"
#include "archive/zip.hpp"
#include "archive/seven_zip.hpp"
#include "archive/tar.hpp"
#include "network/curl.hpp"
#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"

URL::URL() :
  m_protocol(),
  m_payload(),
  m_plugin(),
  m_plugin_payload()
{
}

URL::~URL()
{
}

URL
URL::from_filename(const std::string& filename)
{
  URL url;
  url.m_protocol = "file";
  url.m_payload  = Filesystem::realpath(filename);
  return url;
}

URL
URL::from_string(const std::string& url)
{
  URL ret;

  std::string::size_type i = url.find_first_of("//");
  assert(i != std::string::npos);

  ret.m_protocol = url.substr(0, i-1);
  std::string::size_type j = url.find("//", i+2);
  if (j == std::string::npos)
  { // no plugin given
    ret.m_payload = url.substr(i+2);
  }
  else
  {
    ret.m_payload  = url.substr(i+2, j-i-2);
    std::string::size_type k = url.find(':', j+2);
    ret.m_plugin          = url.substr(j+2, k-j-2);
    ret.m_plugin_payload  = url.substr(k+1);
    //std::cout << "'" << protocol << "' '" << payload << "' '" << plugin << "' " << plugin_payload << std::endl;
  }

  return ret;
}

std::string
URL::str() const
{
  std::string url = m_protocol + "://" + m_payload;
  if (!m_plugin.empty())
  {
    return url + "//" + m_plugin + ":" + m_plugin_payload;
  }
  else
  {
    return url;
  }
}

std::string
URL::get_stdio_name() const
{
  assert(has_stdio_name());
  return m_payload;
}

bool
URL::has_stdio_name() const
{
  return m_protocol == "file" && m_plugin.empty();
}

std::string
URL::get_protocol() const
{
  return m_protocol;
}

std::string
URL::get_payload() const
{
  return m_payload;
}

Blob
URL::get_blob(std::string* mime_type) const
{
  if (m_protocol == "file")
  {
    if (m_plugin.empty())
    {
      return Blob::from_file(m_payload);
    }
    else if (m_plugin == "rar")
    {
      return Rar::get_file(m_payload, m_plugin_payload);
    }
    else if (m_plugin == "zip")
    {
      return Zip::get_file(m_payload, m_plugin_payload);
    }
    else if (m_plugin == "7zip")
    {
      return SevenZip::get_file(m_payload, m_plugin_payload);
    }
    else if (m_plugin == "tar")
    {
      return Tar::get_file(m_payload, m_plugin_payload);
    }
    else
    {
      raise_runtime_error("URL::get_blob(): Unhandled plugin: " + m_plugin);
    }
  }
  else if (m_protocol == "http" || m_protocol == "https" || m_protocol == "ftp")
  {
    return CURLHandler::get_data(str(), mime_type);
  }
  else
  {
    raise_runtime_error("URL::get_blob(): Unhandled protocol: " + m_protocol);
    return {};
  }
}

time_t
URL::get_mtime() const
{
  if (has_stdio_name())
  {
    return Filesystem::get_mtime(get_stdio_name());
  }
  else
  {
    return 0;
  }
}

size_t
URL::get_size() const
{
  if (has_stdio_name())
  {
    return Filesystem::get_size(get_stdio_name());
  }
  else
  {
    return 0;
  }
}

bool
URL::is_url(const std::string& url)
{
  std::string::size_type k = url.find("://");

  if (k == std::string::npos)
  {
    return false;
  }
  else
  {
    for(std::string::size_type i = 0; i < k; ++i) {
      if (!(url[i] >= 'a' && url[i] <= 'z')) {
        return false;
      }
    }

    return true;
  }
}

bool
URL::is_remote() const
{
  return m_protocol != "file";
}

std::ostream& operator<<(std::ostream& out, const URL& url)
{
  return out << "URL(\"" << url.str() << "\")";
}

#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"

bool operator<(const URL& lhs, const URL& rhs)
{
  return lhs.str() < rhs.str();
}

bool operator==(const URL& lhs, const URL& rhs)
{
  return (lhs.m_payload  == rhs.m_payload  &&
          lhs.m_protocol == rhs.m_protocol &&
          lhs.m_plugin   == rhs.m_plugin   &&
          lhs.m_plugin_payload == rhs.m_plugin_payload);
}

/* EOF */
