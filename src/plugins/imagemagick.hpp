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

#ifndef HEADER_GALAPIX_PLUGINS_IMAGEMAGICK_HPP
#define HEADER_GALAPIX_PLUGINS_IMAGEMAGICK_HPP

#include <string>

#include "util/software_surface.hpp"

class URL;
class Size;

class Imagemagick
{
public:
  static bool get_size(const std::string& filename, Size& size);
  static SoftwareSurfacePtr load_from_file(const std::string& filename);
  static SoftwareSurfacePtr load_from_mem(void* data, int len);
  static SoftwareSurfacePtr load_from_url(const URL& url);
  static std::vector<std::string> get_supported_extensions();
};

#endif

/* EOF */
