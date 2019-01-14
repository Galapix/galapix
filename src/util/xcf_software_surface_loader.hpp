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

#ifndef HEADER_GALAPIX_UTIL_XCF_SOFTWARE_SURFACE_LOADER_HPP
#define HEADER_GALAPIX_UTIL_XCF_SOFTWARE_SURFACE_LOADER_HPP

#include "util/software_surface_factory.hpp"

class XCFSoftwareSurfaceLoader : public SoftwareSurfaceLoader
{
private:
public:
  XCFSoftwareSurfaceLoader()
  {}

  std::string get_name() const override
  {
    return "xcf";
  }

  void register_loader(SoftwareSurfaceFactory& factory) const override
  {
    factory.register_by_extension(this, "xcf");
    factory.register_by_extension(this, "xcf.bz2");
    factory.register_by_extension(this, "xcf.gz");

    factory.register_by_mime_type(this, "application/x-gimp-image");

    factory.register_by_magic(this, "gimp xcf");
  }

  bool supports_from_file() const override { return true; }
  bool supports_from_mem()  const override { return true; }

  SoftwareSurface from_file(const std::string& filename) const override
  {
    return XCF::load_from_file(filename);
  }

  SoftwareSurface from_mem(const uint8_t* data, size_t len) const override
  {
    return XCF::load_from_mem(data, len);
  }

private:
  XCFSoftwareSurfaceLoader(const XCFSoftwareSurfaceLoader&);
  XCFSoftwareSurfaceLoader& operator=(const XCFSoftwareSurfaceLoader&);
};

#endif

/* EOF */
