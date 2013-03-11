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

ResourceHandler
ResourceHandler::from_string(const std::string& handler)
{
  std::string::size_type p = handler.find(':');

  ResourceHandler result;
  result.name = handler.substr(0, p);

  while(p != std::string::npos)
  {
    std::string::size_type beg = p+1;
    p = handler.find(':', beg);
    result.args.emplace_back(handler.substr(beg, p - beg));
  }

  return result;
}

ResourceLocator
ResourceLocator::from_string(const std::string& locator)
{
  ResourceURL url = ResourceURL::from_string(locator);
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

/* EOF */
