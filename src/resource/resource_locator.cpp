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

#include "resource_locator.hpp"

#include <boost/algorithm/string/predicate.hpp>
#include <sstream>

ResourceLocator
ResourceLocator::from_string(const std::string& locator)
{
  ResourceURL url;

  // FIXME: rewrite this whole thing, use regex or something

  if (boost::starts_with(locator, "file:") ||
      boost::starts_with(locator, "ftp:")  ||
      boost::starts_with(locator, "http:") ||
      boost::starts_with(locator, "https:"))
  {
    url = ResourceURL::from_string(locator);
  }
  else
  {
    url = ResourceURL("file", std::string(), locator);
  }

  std::string path = url.get_path();
  std::string::size_type handler_start = path.find("//", 0, 2);
  if (handler_start == std::string::npos)
  {
    return ResourceLocator(url, std::vector<ResourceHandler>());
  }
  else
  {
    std::string::size_type beg = handler_start;

    std::vector<ResourceHandler> handler;
    do
    {
      beg += 2;
      std::string::size_type end = path.find("//", beg + 2, 2);
      if (end == std::string::npos)
      {
        handler.push_back(ResourceHandler::from_string(path.substr(beg)));
      }
      else
      {
        handler.push_back(ResourceHandler::from_string(path.substr(beg, end - beg)));
      }
      beg = end;
    } 
    while(beg != std::string::npos);

    return ResourceLocator(ResourceURL(url.get_scheme(),
                                       url.get_authority(),
                                       path.substr(0, handler_start)),
                           handler);
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

std::string
ResourceLocator::str() const
{
  std::ostringstream out;
  out << m_url.str();
  for(const auto& handler : m_handler)
  {
    out << "//" << handler.get_type() << "-" << handler.get_name();
    for(const auto& arg : handler.get_args())
    {
      out << ':' << arg;
    }
  }
  return out.str();
}

/* EOF */
