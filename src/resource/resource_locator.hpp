/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GALAPIX_RESOURCE_LOCATOR_HPP
#define HEADER_GALAPIX_GALAPIX_RESOURCE_LOCATOR_HPP

#include <string>
#include <vector>

#include "resource/resource_url.hpp"

class ResourceHandler
{
public:
  std::string type;
  std::string name;
  std::vector<std::string> args;

  ResourceHandler() : type(), name(), args() {}

  static ResourceHandler from_string(const std::string& handler);
};

/**
   The ResourceLocator provides a path to a retrievable resource, most
   commonly this will be a file path or web URL. The {handler} part
   allows to dig into a resource and address child objects, such as
   files inside an archive.

   The ResourceLocator is represented as string in the form:
    
   "{protocol}://{path}//{handler}:{args}"

    Example:
    
    "http://www.example.org/test.zip//zip:image.jpg"
    "http://www.example.org/test.pdf//pdf:5"
 */
class ResourceLocator
{
private:
  ResourceURL m_url;
  std::vector<ResourceHandler> m_handler;

public:
  /** Takes a filename in the form of a string and transforms it into
      a ResourceLocator */
  static ResourceLocator from_string(const std::string& str);

public:
  ResourceLocator() : m_url(), m_handler() {}
  ResourceLocator(const ResourceURL& url, const std::vector<ResourceHandler>& handler) :
    m_url(url),
    m_handler(handler)
  {}

  const ResourceURL& get_url() const { return m_url; }
  const std::vector<ResourceHandler>& get_handler() const { return m_handler; }
  std::string get_type() const;

  std::string str() const;
};

#endif

/* EOF */
