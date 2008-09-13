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

#include <png.h>
#include "png.hpp"

bool
PNG::get_size(const std::string& filename, Size& size)
{
  // FIXME: Could install error/warning handling functions here 
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    {
      return false;
    }
  else
    {
      png_init_io(png_ptr, in);

      png_read_info(png_ptr, info_ptr); 

      size.width  = png_get_image_width(png_ptr, info_ptr);
      size.height = png_get_image_height(png_ptr, info_ptr);

      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

      fclose(in);
        
      return true;
    }
}

SoftwareSurface
PNG::load_from_file(const std::string& filename)
{
  assert(!"Unimplemented");
  return SoftwareSurface();
}

SoftwareSurface
PNG::load_from_mem(uint8_t* mem, int len)
{
  assert(!"Unimplemented");
  return SoftwareSurface();
}

void
PNG::save(const SoftwareSurface& surface, int quality, const std::string& filename)
{
  assert(!"Unimplemented");
}

Blob
PNG::save(const SoftwareSurface& surface, int quality)
{
  assert(!"Unimplemented");
  return Blob();
}


/* EOF */
