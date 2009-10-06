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

#include "plugins/png.hpp"

#include <png.h>
#include <iostream>
#include <stdexcept>

struct PNGReadMemory
{
  png_bytep  data;
  png_size_t len;
  png_size_t pos;
};

void readPNGMemory(png_structp png_ptr, png_bytep data, png_size_t length)
{
  PNGReadMemory* mem = static_cast<PNGReadMemory*>(png_ptr->io_ptr);

  if (mem->pos + length > mem->len)
    {
      png_error(png_ptr, "PNG: readPNGMemory: Read Error");
    }
  else
    {
      memcpy(data, mem->data + mem->pos, length);
      mem->pos += length;
    }
}

bool
PNG::get_size(void* data, int len, Size& size)
{ 
  // FIXME: Could install error/warning handling functions here 
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  if (setjmp(png_ptr->jmpbuf))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      std::cout << "PNG::get_size: setjmp: Couldn't load from memory" << std::endl;
      return false;
    }

  PNGReadMemory png_memory;
  png_memory.data = (png_bytep)data;
  png_memory.len  = len;
  png_memory.pos  = 0;

  png_set_read_fn(png_ptr, &png_memory, &readPNGMemory);

  png_read_info(png_ptr, info_ptr); 

  size.width  = png_get_image_width(png_ptr, info_ptr);
  size.height = png_get_image_height(png_ptr, info_ptr);

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return true;
}

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

      if (setjmp(png_ptr->jmpbuf))
        {
          png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
          std::cout << "PNG::get_size: setjmp: Couldn't load " << filename << std::endl;
          return false;
        }

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

      if (setjmp(png_ptr->jmpbuf))
        {
          png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
          throw std::runtime_error("PNG::load_from_mem: setjmp: Couldn't load " + filename);
        }

      png_init_io(png_ptr, in);

      png_read_info(png_ptr, info_ptr); 
      
      // Convert all formats to either RGB or RGBA so we don't have to
      // handle them all seperatly
      png_set_strip_16(png_ptr);
      png_set_expand_gray_1_2_4_to_8(png_ptr);
      png_set_palette_to_rgb(png_ptr);
      png_set_expand(png_ptr); // FIXME: What does this do? what the other don't?
      png_set_tRNS_to_alpha(png_ptr);
      png_set_gray_to_rgb(png_ptr);

      png_read_update_info(png_ptr, info_ptr);

      int width  = png_get_image_width(png_ptr, info_ptr);
      int height = png_get_image_height(png_ptr, info_ptr);
      //int pitch  = png_get_rowbytes(png_ptr, info_ptr);

      SoftwareSurface surface;

      switch(png_get_color_type(png_ptr, info_ptr))
        {
          case PNG_COLOR_TYPE_RGBA:
            {
              surface = SoftwareSurface(SoftwareSurface::RGBA_FORMAT, Size(width, height));

              boost::scoped_array<png_bytep> row_pointers(new png_bytep[height]);
              for (int y = 0; y < height; ++y)
                row_pointers[y] = surface.get_row_data(y);
            
              png_read_image(png_ptr, row_pointers.get());
            }
            break;           

          case PNG_COLOR_TYPE_RGB:
            {
              surface = SoftwareSurface(SoftwareSurface::RGB_FORMAT, Size(width, height));

              boost::scoped_array<png_bytep> row_pointers(new png_bytep[height]);
              for (int y = 0; y < height; ++y)
                row_pointers[y] = surface.get_row_data(y);
            
              png_read_image(png_ptr, row_pointers.get());
            }
            break;
        }

      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

      fclose(in);
        
      return surface;
    }
}

SoftwareSurface
PNG::load_from_mem(uint8_t* data, int len)
{
  // FIXME: Merge this with load_from_file
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  PNGReadMemory png_memory;
  png_memory.data = data;
  png_memory.len  = len;
  png_memory.pos  = 0;

  if (setjmp(png_ptr->jmpbuf))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      throw std::runtime_error("PNG::load_from_mem: setjmp: Couldn't load from mem");
    }

  png_set_read_fn(png_ptr, &png_memory, &readPNGMemory);

  png_read_info(png_ptr, info_ptr); 
      
  // Convert all formats to either RGB or RGBA so we don't have to
  // handle them all seperatly
  png_set_strip_16(png_ptr);
  png_set_expand_gray_1_2_4_to_8(png_ptr);
  png_set_palette_to_rgb(png_ptr);
  png_set_expand(png_ptr); // FIXME: What does this do? what the other don't?
  png_set_tRNS_to_alpha(png_ptr);
  png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  int width  = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  //int pitch  = png_get_rowbytes(png_ptr, info_ptr);

  SoftwareSurface surface;

  switch(png_get_color_type(png_ptr, info_ptr))
    {
      case PNG_COLOR_TYPE_RGBA:
        {
          surface = SoftwareSurface(SoftwareSurface::RGBA_FORMAT, Size(width, height));

          boost::scoped_array<png_bytep> row_pointers(new png_bytep[height]);
          for (int y = 0; y < height; ++y)
            row_pointers[y] = surface.get_row_data(y);
            
          png_read_image(png_ptr, row_pointers.get());
        }
        break;           

      case PNG_COLOR_TYPE_RGB:
        {
          surface = SoftwareSurface(SoftwareSurface::RGB_FORMAT, Size(width, height));
          
          boost::scoped_array<png_bytep> row_pointers(new png_bytep[height]);
          for (int y = 0; y < height; ++y)
            row_pointers[y] = surface.get_row_data(y);
            
          png_read_image(png_ptr, row_pointers.get());
        }
        break;
    }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

  return surface;
}

void
PNG::save(const SoftwareSurface& surface, const std::string& filename)
{
  FILE* out = fopen(filename.c_str(), "wb");
  if (!out)
    {
      perror(filename.c_str());
      throw std::runtime_error("PNG::save: Couldn't save " + filename);
    }
  else
    {
      png_structp png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
      png_infop   info_ptr = png_create_info_struct(png_ptr);
      assert(png_ptr && info_ptr);

      if (setjmp(png_ptr->jmpbuf))
        {
          fclose(out);
          png_destroy_write_struct(&png_ptr, &info_ptr);
          throw std::runtime_error("PNG::save: setjmp: Couldn't save " + filename);
        }

      // set up the output control if you are using standard C streams
      png_init_io(png_ptr, out);

      png_set_IHDR(png_ptr, info_ptr, 
                   surface.get_width(), surface.get_height(), 8,
                   (surface.get_format() == SoftwareSurface::RGB_FORMAT) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA, 
                   PNG_INTERLACE_NONE, 
                   PNG_COMPRESSION_TYPE_DEFAULT,
                   PNG_FILTER_TYPE_DEFAULT);

      png_write_info(png_ptr, info_ptr);

      for (int y = 0; y < surface.get_height(); ++y)
        png_write_row(png_ptr, surface.get_row_data(y));

      png_write_end(png_ptr, info_ptr);

      png_destroy_write_struct(&png_ptr, &info_ptr);

      fclose(out);
    }
}

class PNGWriteMemory
{
public:
  std::vector<uint8_t> data;

  PNGWriteMemory() :
    data()
  {}
};

void writePNGMemory(png_structp png_ptr, png_bytep data, png_size_t length)
{
  PNGWriteMemory* mem = static_cast<PNGWriteMemory*>(png_ptr->io_ptr);
  std::copy(data, data+length, std::back_inserter(mem->data));
}

Blob
PNG::save(const SoftwareSurface& surface)
{
  // FIXME: Merge this with the save to file function
  png_structp png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_infop   info_ptr = png_create_info_struct(png_ptr);
  assert(png_ptr && info_ptr);

  if (setjmp(png_ptr->jmpbuf))
    {
      png_destroy_write_struct(&png_ptr, &info_ptr);
      throw std::runtime_error("PNG::save: setjmp: Couldn't save to Blob");
    }

  PNGWriteMemory mem;
  png_set_write_fn(png_ptr, &mem, &writePNGMemory, NULL);

  png_set_IHDR(png_ptr, info_ptr, 
               surface.get_width(), surface.get_height(), 8,
               (surface.get_format() == SoftwareSurface::RGB_FORMAT) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA, 
               PNG_INTERLACE_NONE, 
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

  for (int y = 0; y < surface.get_height(); ++y)
    png_write_row(png_ptr, surface.get_row_data(y));

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  return Blob::copy(mem.data);
}


/* EOF */
