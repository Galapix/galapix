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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_LOCATOR_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_LOCATOR_HPP

#include <string>
#include <vector>

#include "resource/resource_handler.hpp"
#include "resource/resource_url.hpp"

/** The ResourceLocator provides a path to a retrievable resource, most
    commonly this will be a file path or web URL. The {handler} part
    allows to dig into a resource and address child objects, such as
    files inside an archive.

    The ResourceLocator is represented as string in the form:

    "{protocol}://{path}//{handler}:{args}"

    Example:

    "http://www.example.org/test.zip//zip:image.jpg"
    "http://www.example.org/test.pdf//pdf:5"  */
class ResourceLocator
{
public:
  /** Takes a filename in the form of a string and transforms it into
      a ResourceLocator */
  static ResourceLocator from_string(const std::string& locator);

public:
  ResourceLocator() : m_url(), m_handler() {}
  ResourceLocator(ResourceURL url, std::vector<ResourceHandler> handler) :
    m_url(std::move(url)),
    m_handler(std::move(handler))
  {}

  ResourceLocator get_blob_locator() const;
  const ResourceURL& get_url() const { return m_url; }
  const std::vector<ResourceHandler>& get_handler() const { return m_handler; }
  std::string get_type() const;

  bool is_parent_of(const ResourceLocator& other) const;

  std::string str() const;

private:
  ResourceURL m_url;
  std::vector<ResourceHandler> m_handler;
};

#endif

/* EOF */
