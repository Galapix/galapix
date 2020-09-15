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

#include "plugins/dds.hpp"

#include <errno.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <sstream>

#include "plugins/dds_surface.hpp"
#include "util/raise_exception.hpp"

bool
DDS::get_size(const std::string& filename, Size& size)
{
  std::ifstream in(filename.c_str(), std::ios::binary);
  if (!in)
  {
    // raise_runtime_error(strerror(errno));
    return false;
  }
  else
  {
    // FIXME: not very fast as we decode the complete surface
    DDSSurface surface(in);
    size = Size(surface.get_width(), surface.get_height());
    return true;
  }
}

SoftwareSurface
DDS::load_from_file(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::binary);
  if (!in)
  {
    // raise_runtime_error(strerror(errno));
    return {};
  }
  else
  {
    DDSSurface dds(in);

    PixelData dst(PixelData::RGBA_FORMAT, Size(dds.get_width(), dds.get_height()));

    if (static_cast<int>(dds.get_length()) != dst.get_width() * dst.get_height() * 4)
    {
      std::ostringstream out;
      out << "DDS::load_from_file(): length missmatch " << dds.get_length()
          << " - " << dst.get_width() << "x" << dst.get_height();
      raise_runtime_error(out.str());
    }

    memcpy(dst.get_data(), dds.get_data(), dds.get_length());

    return SoftwareSurface(std::move(dst));
  }
}

/* EOF */
