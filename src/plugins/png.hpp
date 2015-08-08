/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_PLUGINS_PNG_HPP
#define HEADER_GALAPIX_PLUGINS_PNG_HPP

#include <string>

#include "math/size.hpp"
#include "util/software_surface.hpp"

class PNG
{
private:
public:
  static bool get_size(void* data, int len, Size& size);
  static bool get_size(const std::string& filename, Size& size);

  static bool is_png(const std::string& filename);

  static SoftwareSurface load_from_file(const std::string& filename);
  static SoftwareSurface load_from_mem(const uint8_t* data, size_t len);

  static void save(SoftwareSurface const& surface, const std::string& filename);
  static Blob save(SoftwareSurface const& surface);
};

#endif

/* EOF */
