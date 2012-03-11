/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_PLUGINS_DDS_HPP
#define HEADER_GALAPIX_PLUGINS_DDS_HPP

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
    size.width  = surface.get_width();
    size.height = surface.get_height();
    return true;
  }
}

SoftwareSurfacePtr
DDS::load_from_file(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::binary);
  if (!in)
  {
    // raise_runtime_error(strerror(errno));
    return SoftwareSurfacePtr();
  }
  else
  {
    DDSSurface dds(in);

    SoftwareSurfacePtr surface 
      = SoftwareSurface::create(SoftwareSurface::RGBA_FORMAT, Size(dds.get_width(), dds.get_height()));
    
    if (dds.get_length() != surface->get_width() * surface->get_height() * 4)
    {
      std::ostringstream out;
      out << "DDS::load_from_file(): length missmatch " << dds.get_length() 
          << " - " << surface->get_width() << "x" << surface->get_height();
      raise_runtime_error(out.str());
    }

    memcpy(surface->get_data(), dds.get_data(), dds.get_length());

    return surface;
  }
}

#endif 

/* EOF */
