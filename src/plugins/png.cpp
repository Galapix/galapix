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

#include "plugins/png.hpp"

#include <string.h>
#include <assert.h>
#include <png.h>
#include <stdexcept>
#include <logmich/log.hpp>

#include "util/raise_exception.hpp"

namespace {

struct PNGReadMemory
{
  png_byte const* data;
  png_size_t len;
  png_size_t pos;
};

void readPNGMemory(png_structp png_ptr, png_bytep data, png_size_t length)
{
  PNGReadMemory* mem = static_cast<PNGReadMemory*>(png_get_io_ptr(png_ptr));

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
  PNGWriteMemory* mem = static_cast<PNGWriteMemory*>(png_get_io_ptr(png_ptr));
  std::copy(data, data+length, std::back_inserter(mem->data));
}

} // namespace

bool
PNG::get_size(void* data, int len, Size& size)
{
  // FIXME: Could install error/warning handling functions here
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    // FIXME: get a proper error message from libpng
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    log_warn("PNG::get_size: setjmp: Couldn't load from memory");
    return false;
  }
  else
  {
    PNGReadMemory png_memory;
    png_memory.data = static_cast<png_bytep>(data);
    png_memory.len = static_cast<png_size_t>(len);
    png_memory.pos = 0;

    png_set_read_fn(png_ptr, &png_memory, &readPNGMemory);

    png_read_info(png_ptr, info_ptr);

    size = Size(static_cast<int>(png_get_image_width(png_ptr, info_ptr)),
                static_cast<int>(png_get_image_height(png_ptr, info_ptr)));

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    return true;
  }
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
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr  = png_create_info_struct(png_ptr);

    if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
      log_warn("PNG::get_size: setjmp: Couldn't load {}", filename);
      return false;
    }
    else
    {
      png_init_io(png_ptr, in);

      png_read_info(png_ptr, info_ptr);

      size = Size(static_cast<int>(png_get_image_width(png_ptr, info_ptr)),
                  static_cast<int>(png_get_image_height(png_ptr, info_ptr)));

      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

      fclose(in);

      return true;
    }
  }
}

bool
PNG::is_png(const std::string& filename)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
  {
    return false;
  }
  else
  {
    unsigned char buf[4];
    if (fread(buf, 1, sizeof(buf), in) != 4)
    {
      fclose(in);
      return false;
    }
    else
    {
      fclose(in);
      static unsigned char magic[4] = { 0x89, 0x50, 0x4e, 0x47 };
      return
        buf[0] == magic[0] &&
        buf[1] == magic[1] &&
        buf[2] == magic[2] &&
        buf[3] == magic[3];
    }
  }
}

SoftwareSurface
PNG::load_from_file(const std::string& filename)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
  {
    raise_runtime_error("PNG::load_from_file(): Couldn't open " + filename);
  }
  else
  {
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr  = png_create_info_struct(png_ptr);

    if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
      raise_runtime_error("PNG::load_from_mem(): setjmp: Couldn't load " + filename);
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

    int width  = static_cast<int>(png_get_image_width(png_ptr, info_ptr));
    int height = static_cast<int>(png_get_image_height(png_ptr, info_ptr));
    //int pitch  = png_get_rowbytes(png_ptr, info_ptr);

    PixelData dst;

    switch(png_get_color_type(png_ptr, info_ptr))
    {
      case PNG_COLOR_TYPE_RGBA:
      {
        dst = PixelData(PixelData::RGBA_FORMAT, Size(width, height));

        std::vector<png_bytep> row_pointers(static_cast<size_t>(height));
        for (int y = 0; y < height; ++y) {
          row_pointers[static_cast<size_t>(y)] = dst.get_row_data(y);
        }

        png_read_image(png_ptr, row_pointers.data());
      }
      break;

      case PNG_COLOR_TYPE_RGB:
      {
        dst = PixelData(PixelData::RGB_FORMAT, Size(width, height));

        std::vector<png_bytep> row_pointers(static_cast<size_t>(height));
        for (int y = 0; y < height; ++y) {
          row_pointers[static_cast<size_t>(y)] = dst.get_row_data(y);
        }

        png_read_image(png_ptr, row_pointers.data());
      }
      break;
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    fclose(in);

    return SoftwareSurface(std::move(dst));
  }
}

SoftwareSurface
PNG::load_from_mem(std::span<uint8_t const> data)
{
  // FIXME: Merge this with load_from_file
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  PNGReadMemory png_memory;
  png_memory.data = data.data();
  png_memory.len  = data.size();
  png_memory.pos  = 0;

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    raise_runtime_error("PNG::load_from_mem(): setjmp: Couldn't load from mem");
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

  int width = static_cast<int>(png_get_image_width(png_ptr, info_ptr));
  int height = static_cast<int>(png_get_image_height(png_ptr, info_ptr));
  //int pitch  = png_get_rowbytes(png_ptr, info_ptr);

  PixelData dst;

  switch(png_get_color_type(png_ptr, info_ptr))
  {
    case PNG_COLOR_TYPE_RGBA:
    {
      dst = PixelData(PixelData::RGBA_FORMAT, Size(width, height));

      std::vector<png_bytep> row_pointers(static_cast<size_t>(height));
      for (int y = 0; y < height; ++y) {
        row_pointers[static_cast<size_t>(y)] = dst.get_row_data(y);
      }

      png_read_image(png_ptr, row_pointers.data());
    }
    break;

    case PNG_COLOR_TYPE_RGB:
    {
      dst = PixelData(PixelData::RGB_FORMAT, Size(width, height));

      std::vector<png_bytep> row_pointers(static_cast<size_t>(height));
      for (int y = 0; y < height; ++y) {
        row_pointers[static_cast<size_t>(y)] = dst.get_row_data(y);
      }

      png_read_image(png_ptr, row_pointers.data());
    }
    break;
  }

  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

  return SoftwareSurface(std::move(dst));
}

void
PNG::save(SoftwareSurface const& surface, const std::string& filename)
{
  PixelData const& src = surface.get_pixel_data();

  FILE* out = fopen(filename.c_str(), "wb");
  if (!out)
  {
    perror(filename.c_str());
    raise_runtime_error("PNG::save(): Couldn't save " + filename);
  }
  else
  {
    png_structp png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop   info_ptr = png_create_info_struct(png_ptr);
    assert(png_ptr && info_ptr);

    if (setjmp(png_jmpbuf(png_ptr)))
    {
      fclose(out);
      png_destroy_write_struct(&png_ptr, &info_ptr);
      raise_runtime_error("PNG::save(): setjmp: Couldn't save " + filename);
    }

    // set up the output control if you are using standard C streams
    png_init_io(png_ptr, out);

    png_set_IHDR(png_ptr, info_ptr,
                 static_cast<png_uint_32>(src.get_width()),
                 static_cast<png_uint_32>(src.get_height()),
                 8,
                 (src.get_format() == PixelData::RGB_FORMAT) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);

    for (int y = 0; y < src.get_height(); ++y) {
      png_write_row(png_ptr, const_cast<png_bytep>(src.get_row_data(y)));
    }

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(out);
  }
}

Blob
PNG::save(SoftwareSurface const& surface)
{
  PixelData const& src = surface.get_pixel_data();

  // FIXME: Merge this with the save to file function
  png_structp png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop   info_ptr = png_create_info_struct(png_ptr);
  assert(png_ptr && info_ptr);

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    raise_runtime_error("PNG::save(): setjmp: Couldn't save to Blob");
  }

  PNGWriteMemory mem;
  png_set_write_fn(png_ptr, &mem, &writePNGMemory, nullptr);

  png_set_IHDR(png_ptr, info_ptr,
               static_cast<png_uint_32>(src.get_width()),
               static_cast<png_uint_32>(src.get_height()),
               8,
               (src.get_format() == PixelData::RGB_FORMAT) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);

  for (int y = 0; y < src.get_height(); ++y)
  {
    png_write_row(png_ptr, const_cast<png_bytep>(src.get_row_data(y)));
  }

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  return Blob::copy(mem.data);
}


/* EOF */
