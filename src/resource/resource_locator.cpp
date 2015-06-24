/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#include "resource_locator.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <sstream>
#include <logmich/log.hpp>

ResourceLocator
ResourceLocator::from_string(const std::string& locator)
{
  std::string::size_type skip = 0;
  if      (boost::starts_with(locator, "file://" )) { skip = 7; }
  else if (boost::starts_with(locator, "ftp://"  )) { skip = 6; }
  else if (boost::starts_with(locator, "http://" )) { skip = 7; }
  else if (boost::starts_with(locator, "https://")) { skip = 8; }
  else                                              { skip = 0; }

  std::string::size_type handler_start = locator.find("//", skip, 2);
  if (handler_start == std::string::npos)
  {
    if (skip == 0)
    {
      return ResourceLocator(ResourceURL("file", std::string(), locator), std::vector<ResourceHandler>());
    }
    else
    {
      return ResourceLocator(ResourceURL::from_string(locator), std::vector<ResourceHandler>());
    }
  }
  else
  {
    std::string path = locator.substr(skip, handler_start - skip);
    ResourceURL url = ResourceURL("file", std::string(), path);
    std::vector<ResourceHandler> handler;

    std::string::size_type beg = handler_start;
    do
    {
      beg += 2;
      std::string::size_type end = locator.find("//", beg, 2);
      if (end == std::string::npos)
      {
        handler.push_back(ResourceHandler::from_string(locator.substr(beg, end - beg)));
      }
      else
      {
        handler.push_back(ResourceHandler::from_string(locator.substr(beg, end - beg)));
      }
      beg = end;
    }
    while(beg != std::string::npos);

    return ResourceLocator(url, handler);
  }
}

std::string
ResourceLocator::get_type() const
{
  if (m_handler.empty())
  {
    return "blob";
  }
  else
  {
    return m_handler.back().get_type();
  }
}

ResourceLocator
ResourceLocator::get_blob_locator() const
{
  ResourceLocator blob_locator(*this);
  while(blob_locator.get_type() != "blob")
  {
    blob_locator.m_handler.pop_back();
  }
  return blob_locator;
}

bool
ResourceLocator::is_parent_of(const ResourceLocator& other) const
{
  /*
    Problem:

    "file://foo.rar" is a blob without a type
    "file://foo.rar//archive-rar" is parent of "file://foo.rar//archive-rar:foo.png"
    "file://foo.rar" is parent of "file://foo.rar//archive-rar:foo.png"
    "file://foo.rar" is also parent of "file://foo.rar//archive-zip:foo.png"

    The last one might however never be catched
  */
  if (m_url == other.m_url)
  {
    if (m_handler.size() <= other.m_handler.size())
    {
      for(std::vector<ResourceHandler>::size_type i = 0; i < m_handler.size(); ++i)
      {
        if (m_handler[i] != other.m_handler[i])
        {
          return false;
        }
      }
      return true;
    }
    else
    {
      // 'this' has more handler then 'other'
      return false;
    }
  }
  else
  {
    // urls don't match
    return false;
  }
}

std::string
ResourceLocator::str() const
{
  std::ostringstream out;
  out << m_url.str();
  for(const auto& handler : m_handler)
  {
    out << "//" << handler.get_type() << "-" << handler.get_name();
    if (!handler.get_args().empty())
    {
      out << ':' << handler.get_args();
    }
  }
  return out.str();
}

/* EOF */
