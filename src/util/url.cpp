/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#include "util/url.hpp"

#include <stdexcept>
#include <ostream>
#include "plugins/rar.hpp"
#include "plugins/zip.hpp"
#include "plugins/tar.hpp"
#include "plugins/curl.hpp"
#include "util/filesystem.hpp"

URL::URL() :
  protocol(),
  payload(),
  plugin(),
  plugin_payload()
{
}

URL::~URL()
{
}

URL
URL::from_filename(const std::string& filename)
{
  URL url;
  url.protocol = "file";
  url.payload  = Filesystem::realpath(filename);
  return url;
}

URL
URL::from_string(const std::string& url)
{
  URL ret;

  std::string::size_type i = url.find_first_of("//");
  assert(i != std::string::npos);

  ret.protocol = url.substr(0, i-1);
  std::string::size_type j = url.find("//", i+2);
  if (j == std::string::npos)
  { // no plugin given
    ret.payload = url.substr(i+2);
  }
  else
  {
    ret.payload  = url.substr(i+2, j-i-2);
    std::string::size_type k = url.find(":", j+2);
    ret.plugin          = url.substr(j+2, k-j-2);
    ret.plugin_payload  = url.substr(k+1);
    //std::cout << "'" << protocol << "' '" << payload << "' '" << plugin << "' " << plugin_payload << std::endl;
  }
  
  return ret;
}

std::string
URL::str() const
{
  std::string url = protocol + "://" + payload;
  if (!plugin.empty())
  {
    return url + "//" + plugin + ":" + plugin_payload;
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
  return payload;
}

bool
URL::has_stdio_name() const
{
  if (protocol == "file" && plugin.empty())
    return true;
  else
    return false;
}

std::string
URL::get_protocol() const
{
  return protocol;
}

BlobPtr
URL::get_blob() const
{
  if (protocol == "file")
  {
    if (plugin.empty())
    {
      return Blob::from_file(payload);
    }
    else if (plugin == "rar")
    {
      return Rar::get_file(payload, plugin_payload);
    }
    else if (plugin == "zip")
    {
      return Zip::get_file(payload, plugin_payload);
    }
    else if (plugin == "tar")
    {
      return Tar::get_file(payload, plugin_payload);
    }
    else
    {
      throw std::runtime_error("URL::get_blob(): Unhandled plugin: " + plugin);
    }
  }
  else if (protocol == "http" || protocol == "https" || protocol == "ftp")
  {
    return CURLHandler::get_data(str());
  }
  else
  {
    throw std::runtime_error("URL::get_blob(): Unhandled protocol: " + protocol);
    return BlobPtr();
  }
}

int
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

int
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
    for(std::string::size_type i = 0; i < k; ++i)
      if (!(url[i] >= 'a' && url[i] <= 'z'))
        return false;
        
    return true;
  }
}

std::ostream& operator<<(std::ostream& out, const URL& url)
{
  return out << "URL(\"" << url.str() << "\")";
}

bool operator<(const URL& lhs, const URL& rhs)
{
  return lhs.str() < rhs.str();
}

bool operator==(const URL& lhs, const URL& rhs)
{
  return (lhs.payload  == rhs.payload  &&
          lhs.protocol == rhs.protocol &&
          lhs.plugin   == rhs.plugin   &&
          lhs.plugin_payload == rhs.plugin_payload);
}

/* EOF */
