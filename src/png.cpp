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
#include <iostream>
#include <stdexcept>
#include "png.hpp"

bool
PNG::get_size(const std::string& filename, Size& size)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    {
      return false;
    }
  else
    {
      // FIXME: Could install error/warning handling functions here 
      png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      png_infop info_ptr  = png_create_info_struct(png_ptr);

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
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    {
      throw std::runtime_error("JPEG::load_from_file: Couldn't open " + filename);
    }
  else
    {
      png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      png_infop info_ptr  = png_create_info_struct(png_ptr);

      png_init_io(png_ptr, in);

      png_read_info(png_ptr, info_ptr); 

      int width  = png_get_image_width(png_ptr, info_ptr);
      int height = png_get_image_height(png_ptr, info_ptr);
      //int pitch  = png_get_rowbytes(png_ptr, info_ptr);

      // FIXME: Different bitdepths not handled

      SoftwareSurface surface;

      switch(png_get_color_type(png_ptr, info_ptr))
        {
          case PNG_COLOR_TYPE_GRAY:
            {
              surface = SoftwareSurface(Size(width, height), SoftwareSurface::RGB_FORMAT);

              png_bytep row_pointers[height];
              png_byte rows[width*height];
              for(int y = 0; y < height; ++y)
                row_pointers[y] = &rows[width*y];

              png_read_image(png_ptr, (png_byte**)row_pointers);

              for (int y = 0; y < height; ++y)
                {
                  uint8_t* row_pixels = surface.get_row_data(y);

                  for(int x = 0; x < width; ++x)
                    {
                      row_pixels[3*x+0] = row_pointers[y][x];
                      row_pixels[3*x+1] = row_pointers[y][x];
                      row_pixels[3*x+2] = row_pointers[y][x];
                    }
                }
            }
            break;           

          case PNG_COLOR_TYPE_GRAY_ALPHA:
            std::cout << "PNG: Unsupported PNG_COLOR_TYPE_GRAY_ALPHA" << std::endl;
            surface = SoftwareSurface(Size(width, height), SoftwareSurface::RGBA_FORMAT);
            break;           

          case PNG_COLOR_TYPE_PALETTE:
            std::cout << "PNG: Unsupported PNG_COLOR_TYPE_PALETTE" << std::endl;
            surface = SoftwareSurface(Size(width, height), SoftwareSurface::RGB_FORMAT);
            break;           

          case PNG_COLOR_TYPE_RGBA:
            {
              surface = SoftwareSurface(Size(width, height), SoftwareSurface::RGBA_FORMAT);

              png_bytep row_pointers[height];
              for (int y = 0; y < height; ++y)
                row_pointers[y] = surface.get_row_data(y);
            
              png_read_image(png_ptr, row_pointers);
            }
            break;           

          case PNG_COLOR_TYPE_RGB:
            {
              surface = SoftwareSurface(Size(width, height), SoftwareSurface::RGB_FORMAT);

              png_bytep row_pointers[height];
              for (int y = 0; y < height; ++y)
                row_pointers[y] = surface.get_row_data(y);
            
              png_read_image(png_ptr, row_pointers);
            }
            break;
        }

      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

      fclose(in);
        
      return surface;
    }
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
