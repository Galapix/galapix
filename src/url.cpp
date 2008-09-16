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

#include <iostream>
#include <stdexcept>
#include "filesystem.hpp"
#include "rar.hpp"
#include "zip.hpp"
#include "tar.hpp"
#include "url.hpp"

URL::URL()
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
URL::get_url() const
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

Blob
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
          throw std::runtime_error("URL: Unhandled plugin: " + plugin);
        }
    }
  else
    {
      throw std::runtime_error("URL: Unhandled protocol: " + protocol);
      return Blob();
    }
  
  
}

std::ostream& operator<<(std::ostream& out, const URL& url)
{
  return out << url.get_url();
}

bool operator<(const URL& lhs, const URL& rhs)
{
  return lhs.get_url() < rhs.get_url();
}

#ifdef __URL_TEST__

#include <iostream>

int main(int argc, char** argv)
{
  std::string url_str = "file://Test Foo/bla/boing";
  URL url = URL::from_string(url_str);
  
  std::cout << "'" << url_str << "'" << std::endl;
  std::cout << "'" << url.get_protocol() << "'" << std::endl;

  return 0;
}

#endif

/* EOF */
