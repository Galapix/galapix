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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_HANDLER_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_HANDLER_HPP

#include <string>

// FIXME: rename ResourceHandler to BlobHandler?
class ResourceHandler
{
private:
  std::string m_type;
  std::string m_name;
  std::string m_args;

public:
  ResourceHandler() : 
    m_type(), m_name(), m_args() 
  {}

  ResourceHandler(const std::string& type, const std::string& name, const std::string& args = std::string()) : 
    m_type(type), m_name(name), m_args(args)
  {}

  std::string get_type() const { return m_type; }
  std::string get_name() const { return m_name; }
  std::string get_args() const { return m_args; }

  std::string str() const;

  static ResourceHandler from_string(const std::string& handler);

  bool operator==(const ResourceHandler& other) const;
  bool operator!=(const ResourceHandler& other) const;
};

#endif

/* EOF */
