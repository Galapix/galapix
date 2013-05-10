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

#include "resource/resource_handler.hpp"

#include <sstream>

ResourceHandler
ResourceHandler::from_string(const std::string& handler)
{
  std::string::size_type p = handler.find(':');

  ResourceHandler result;
  result.m_name = handler.substr(0, p);
  std::string::size_type minus = result.m_name.find('-');
  if (minus == std::string::npos)
  {
    result.m_type = "unknown";
  }
  else
  {
    result.m_type = result.m_name.substr(0, minus);
    result.m_name = result.m_name.substr(minus+1);
  }

  if(p != std::string::npos)
  {
    result.m_args = handler.substr(p+1);
  }

  return result;
}

std::string
ResourceHandler::str() const
{
  std::ostringstream out;
  out << m_type << '-' << m_name;
  if (!m_args.empty())
  {
    out << ':' << m_args;
  }
  return out.str();
}

/* EOF */
