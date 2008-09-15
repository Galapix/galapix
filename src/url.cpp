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

#include "filesystem.hpp"
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
  url.url = "file://" + Filesystem::realpath(filename);
  return url;
}

URL
URL::from_string(const std::string& str)
{
  URL url;
  url.url = str;
  return url;
}

std::string
URL::get_url() const
{
  return url;
}

std::string
URL::get_stdio_name() const
{
  return url.substr(7);
}

bool
URL::has_stdio_name() const
{
  return true;
}

Blob
URL::get_blob() const
{
  return Blob();
}

/* EOF */
