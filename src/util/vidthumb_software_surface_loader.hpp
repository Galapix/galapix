/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_UTIL_VIDTHUMB_SOFTWARE_SURFACE_LOADER_HPP
#define HEADER_GALAPIX_UTIL_VIDTHUMB_SOFTWARE_SURFACE_LOADER_HPP

#include "plugins/rsvg.hpp"
#include "util/software_surface_loader.hpp"

class VidThumbSoftwareSurfaceLoader : public SoftwareSurfaceLoader
{
private:
public:
  VidThumbSoftwareSurfaceLoader()
  {}

  std::string get_name() const
  {
    return "vidthumb";
  }

  void register_loader(SoftwareSurfaceFactory& factory) const
  {
    factory.register_by_extension(this, "avi");
    factory.register_by_extension(this, "wmv");
    factory.register_by_extension(this, "flv");
    factory.register_by_extension(this, "ogv");
    factory.register_by_extension(this, "mkv");
    factory.register_by_extension(this, "mp4");
  }

  bool supports_from_file() const { return true;  }
  bool supports_from_mem()  const { return false; }

  SoftwareSurfacePtr from_file(const std::string& filename) const
  {
    return VidThumb::load_from_file(filename);
  }

  SoftwareSurfacePtr from_mem(const uint8_t* data, int len) const
  {
    assert(!"not implemented");
  }

private:
  VidThumbSoftwareSurfaceLoader(const VidThumbSoftwareSurfaceLoader&);
  VidThumbSoftwareSurfaceLoader& operator=(const VidThumbSoftwareSurfaceLoader&);
};

#endif

/* EOF */
