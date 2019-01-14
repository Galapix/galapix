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

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_LOADER_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_LOADER_HPP

#include "util/software_surface.hpp"

class SoftwareSurfaceFactory;

class SoftwareSurfaceLoader
{
public:
  SoftwareSurfaceLoader() {}
  virtual ~SoftwareSurfaceLoader() {}

  virtual std::string get_name() const =0;

  virtual void register_loader(SoftwareSurfaceFactory& factory) const =0;

  virtual bool supports_from_file() const =0;
  virtual SoftwareSurface from_file(const std::string& filename) const =0;

  virtual bool supports_from_mem() const =0;
  virtual SoftwareSurface from_mem(const uint8_t* data, size_t len) const =0;

private:
  SoftwareSurfaceLoader(const SoftwareSurfaceLoader&);
  SoftwareSurfaceLoader& operator=(const SoftwareSurfaceLoader&);
};

#endif

/* EOF */
