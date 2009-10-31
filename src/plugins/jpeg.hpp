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

#ifndef HEADER_GALAPIX_PLUGINS_JPEG_HPP
#define HEADER_GALAPIX_PLUGINS_JPEG_HPP

#include <string>
#include <jpeglib.h>
#include <boost/function.hpp>

#include "util/software_surface.hpp"

class JPEG
{
private:

public:
  static bool filename_is_jpeg(const std::string& filename);

  static Size get_size(const std::string& filename);
  static Size get_size(uint8_t* data, int len);

  /** Load a SoftwareSurface from the filesystem
      
      @param[in]  filename Filename of the file to load
      @param[in]  scale    Scale the image by 1/scale (only 1,2,4,8 allowed)
      @param[out] size     The size of the unscaled image

      @return reference counted pointer to a SoftwareSurface object
   */
  static SoftwareSurfacePtr load_from_file(const std::string& filename, int scale = 1, Size* size = NULL);

  /** Load a JPEG from memory 
      
      @param[in]  data  Address of the JPEG data
      @param[in]  len   Length of the JPEG data
      @param[out] size  The size of the unscaled image

      @return reference counted pointer to a SoftwareSurface object
   */
  static SoftwareSurfacePtr load_from_mem(uint8_t* data, int len, int scale = 1, Size* size = NULL);

  static void save(const SoftwareSurfacePtr& surface, int quality, const std::string& filename);
  static BlobPtr save(const SoftwareSurfacePtr& surface, int quality);
};

#endif

/* EOF */
