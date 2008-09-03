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

#ifndef HEADER_JPEG_HPP
#define HEADER_JPEG_HPP

#include <string>
#include <jpeglib.h>
#include <boost/function.hpp>
#include "software_surface.hpp"

class JPEG
{
private:

public:
  static bool get_size(const std::string& filename, Size& size);

  static SoftwareSurface load(const boost::function<void (j_decompress_ptr)>& setup_src_mgr,
                              int scale = 1);

  /** Load a SoftwareSurface from a JPEG file
      
      @param filename   Filename of the file to load
      @param scale      Scale the image by 1/scale (only 1,2,4,8 allowed)
   */
  static SoftwareSurface load_from_file(const std::string& filename, int scale = 1);

  /** Load a JPEG from memory 
      
      @param mem   Address of the JPEG data
      @param len   Length of the JPEG data
   */
  static SoftwareSurface load_from_mem(uint8_t* mem, int len, int scale = 1);

  static void save(const SoftwareSurface& surface, int quality, const std::string& filename);
  static Blob save(const SoftwareSurface& surface, int quality);
};

#endif

/* EOF */
