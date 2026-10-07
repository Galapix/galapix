// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include "plugins/png.hpp"

#include <assert.h>
#include <bit>
#include <memory>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <string.h>

#include <png.h>

#include <logmich/log.hpp>

#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"

namespace surf {
namespace png {

namespace {

struct PNGReadMemory
{
  png_byte const* data;
  png_size_t len;
  png_size_t pos;
};

png_byte PixelFormat2PNG_COLOR_TYPE(PixelFormat format)
{
  switch (format)
  {
    case PixelFormat::RGB8:
    case PixelFormat::RGB16:
      return PNG_COLOR_TYPE_RGB;

    case PixelFormat::RGBA8:
    case PixelFormat::RGBA16:
      return PNG_COLOR_TYPE_RGBA;

    default:
      throw std::invalid_argument("PNG: unhandled format");
  }
}

int PixelFormat2bitdepth(PixelFormat format)
{
  switch (format)
  {
    case PixelFormat::RGB8:
    case PixelFormat::RGBA8:
      return 8;

    case PixelFormat::RGB16:
    case PixelFormat::RGBA16:
      return 16;

    default:
      throw std::invalid_argument("PNG: unhandled format");
  }
}

void readPNGMemory(png_structp png_ptr, png_bytep data, png_size_t length)
{
  PNGReadMemory* mem = static_cast<PNGReadMemory*>(png_get_io_ptr(png_ptr));

  if (mem->pos + length > mem->len) {
    png_error(png_ptr, "PNG: readPNGMemory: Read Error");
  } else {
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

/** Destroys the png read struct when going out of scope */
struct PNGReadStructGuard
{
  png_structp png_ptr;
  png_infop info_ptr;

  PNGReadStructGuard() :
    png_ptr(png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr)),
    info_ptr(png_ptr ? png_create_info_struct(png_ptr) : nullptr)
  {
    if (!png_ptr || !info_ptr) {
      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
      throw std::runtime_error("PNG: failed to create read struct");
    }
  }

  ~PNGReadStructGuard() {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  }

  PNGReadStructGuard(PNGReadStructGuard const&) = delete;
  PNGReadStructGuard& operator=(PNGReadStructGuard const&) = delete;
};

/** PNG stores 16bit samples big-endian, PixelData uses native endian */
void set_swap_16bit(png_structp png_ptr, int bit_depth)
{
  if constexpr (std::endian::native == std::endian::little) {
    if (bit_depth == 16) {
      png_set_swap(png_ptr);
    }
  }
}

} // namespace

bool get_size(void* data, int len, geom::isize& size)
{
  // FIXME: Could install error/warning handling functions here
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  if (setjmp(png_jmpbuf(png_ptr))) {
    // FIXME: get a proper error message from libpng
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    log_warn("PNG::get_size: setjmp: Couldn't load from memory");
    return false;
  } else {
    PNGReadMemory png_memory;
    png_memory.data = static_cast<png_bytep>(data);
    png_memory.len = static_cast<png_size_t>(len);
    png_memory.pos = 0;

    png_set_read_fn(png_ptr, &png_memory, &readPNGMemory);

    png_read_info(png_ptr, info_ptr);

    size = geom::isize(static_cast<int>(png_get_image_width(png_ptr, info_ptr)),
                       static_cast<int>(png_get_image_height(png_ptr, info_ptr)));

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    return true;
  }
}

bool get_size(std::filesystem::path const& filename, geom::isize& size)
{
  std::unique_ptr<FILE, int(*)(FILE*)> in_guard(fopen(filename.string().c_str(), "rb"), &fclose);
  FILE* in = in_guard.get();
  if (!in) {
    return false;
  }

  // FIXME: Could install error/warning handling functions here
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr  = png_create_info_struct(png_ptr);

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    log_warn("PNG::get_size: setjmp: Couldn't load {}", filename.string());
    return false;
  } else {
    png_init_io(png_ptr, in);

    png_read_info(png_ptr, info_ptr);

    size = geom::isize(static_cast<int>(png_get_image_width(png_ptr, info_ptr)),
                       static_cast<int>(png_get_image_height(png_ptr, info_ptr)));

    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

    return true;
  }
}

bool is_png(std::filesystem::path const& filename)
{
  FILE* in = fopen(filename.string().c_str(), "rb");
  if (!in) {
    return false;
  } else {
    unsigned char buf[4];
    if (fread(buf, 1, sizeof(buf), in) != 4) {
      fclose(in);
      return false;
    } else {
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

SoftwareSurface load_from_stream(std::istream& is, std::string const& context)
{
  // Everything with a destructor is declared before setjmp(), as
  // longjmp() would otherwise skip over it
  PNGReadStructGuard png;
  png_structp const png_ptr = png.png_ptr;
  png_infop const info_ptr = png.info_ptr;

  SoftwareSurface surface;
  std::vector<png_bytep> row_pointers;

  if (setjmp(png_jmpbuf(png_ptr))) {
    throw std::runtime_error("PNG::load_from_stream(): setjmp: Couldn't load " + context);
  }

  png_set_read_fn(png_ptr, &is, [](png_structp l_png_ptr, png_bytep buffer, size_t len) {
    std::istream& l_is = *static_cast<std::istream*>(png_get_io_ptr(l_png_ptr));
    if (!l_is.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(len))) {
      png_error(l_png_ptr, "unexpected end of file");
    }
  });

  png_read_info(png_ptr, info_ptr);

  // Convert all formats to either RGB or RGBA so we don't have to
  // handle them all seperatly
  //png_set_strip_16(png_ptr);
  png_set_expand_gray_1_2_4_to_8(png_ptr);
  png_set_palette_to_rgb(png_ptr);
  png_set_expand(png_ptr); // FIXME: What does this do? what the other don't?
  png_set_tRNS_to_alpha(png_ptr);
  png_set_gray_to_rgb(png_ptr);
  set_swap_16bit(png_ptr, png_get_bit_depth(png_ptr, info_ptr));

  png_read_update_info(png_ptr, info_ptr);

  PixelFormat format;
  switch (png_get_bit_depth(png_ptr, info_ptr))
  {
    case 8:
      switch (png_get_color_type(png_ptr, info_ptr))
      {
        case PNG_COLOR_TYPE_RGBA:
          format = PixelFormat::RGBA8;
          break;

        case PNG_COLOR_TYPE_RGB:
          format = PixelFormat::RGB8;
          break;

        default:
          throw std::invalid_argument("invalid png color type");
          break;
      }
      break;

    case 16:
      switch (png_get_color_type(png_ptr, info_ptr))
      {
        case PNG_COLOR_TYPE_RGBA:
          format = PixelFormat::RGBA16;
          break;

        case PNG_COLOR_TYPE_RGB:
          format = PixelFormat::RGB16;
          break;

        default:
          throw std::invalid_argument("invalid png color type");
          break;
      }
      break;

    default:
      throw std::invalid_argument("invalid png bit depth");
      break;
  }

  geom::isize const size(static_cast<int>(png_get_image_width(png_ptr, info_ptr)),
                         static_cast<int>(png_get_image_height(png_ptr, info_ptr)));

  surface = SoftwareSurface::create(format, size);
  row_pointers.resize(static_cast<size_t>(surface.get_height()));
  for (int y = 0; y < surface.get_height(); ++y) {
    row_pointers[static_cast<size_t>(y)] = static_cast<png_bytep>(surface.get_row_data(y));
  }
  png_read_image(png_ptr, row_pointers.data());

  return surface;
}

SoftwareSurface load_from_file(std::filesystem::path const& filename)
{
  std::ifstream fin(filename, std::ios::binary);
  if (!fin) {
    throw std::runtime_error("PNG::load_from_file(): Couldn't open " + filename.string());
  }

  return load_from_stream(fin, filename.string());
}

SoftwareSurface load_from_mem(std::span<uint8_t const> data)
{
  // FIXME: unnecessary copy here
  std::istringstream iss(std::string(data.begin(), data.end()));
  return load_from_stream(iss, "<memory>");
}

void save(SoftwareSurface const& surface, std::filesystem::path const& filename)
{
  FILE* out = fopen(filename.string().c_str(), "wb");
  if (!out)
  {
    perror(filename.string().c_str());
    throw std::runtime_error("PNG::save(): Couldn't save " + filename.string());
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
      throw std::runtime_error("PNG::save(): setjmp: Couldn't save " + filename.string());
    }

    // set up the output control if you are using standard C streams
    png_init_io(png_ptr, out);

    IPixelData const& src = surface.get_pixel_data();
    png_set_IHDR(png_ptr, info_ptr,
                 static_cast<png_uint_32>(src.get_width()),
                 static_cast<png_uint_32>(src.get_height()),
                 PixelFormat2bitdepth(surface.get_format()),
                 PixelFormat2PNG_COLOR_TYPE(surface.get_format()),
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png_ptr, info_ptr);
    set_swap_16bit(png_ptr, PixelFormat2bitdepth(surface.get_format()));

    for (int y = 0; y < src.get_height(); ++y) {
      png_write_row(png_ptr, const_cast<png_bytep>(static_cast<png_byte const*>(src.get_row_data(y))));
    }

    png_write_end(png_ptr, info_ptr);

    png_destroy_write_struct(&png_ptr, &info_ptr);

    fclose(out);
  }
}

std::vector<uint8_t> save(SoftwareSurface const& surface)
{
  // FIXME: Merge this with the save to file function
  png_structp png_ptr  = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop   info_ptr = png_create_info_struct(png_ptr);
  assert(png_ptr && info_ptr);

  if (setjmp(png_jmpbuf(png_ptr)))
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    throw std::runtime_error("PNG::save(): setjmp: Couldn't save to Blob");
  }

  PNGWriteMemory mem;
  png_set_write_fn(png_ptr, &mem, &writePNGMemory, nullptr);

  IPixelData const& src = surface.get_pixel_data();

  png_set_IHDR(png_ptr, info_ptr,
               static_cast<png_uint_32>(src.get_width()),
               static_cast<png_uint_32>(src.get_height()),
               PixelFormat2bitdepth(surface.get_format()),
               PixelFormat2PNG_COLOR_TYPE(surface.get_format()),
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);
  set_swap_16bit(png_ptr, PixelFormat2bitdepth(surface.get_format()));

  for (int y = 0; y < src.get_height(); ++y) {
    png_write_row(png_ptr, const_cast<png_bytep>(static_cast<png_byte const*>(src.get_row_data(y))));
  }

  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  return std::move(mem.data);
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("png", load_from_file, load_from_mem);

  factory.register_by_extension(*loader, "png");

  factory.register_by_magic(*loader, "\x89PNG");

  factory.register_by_mime_type(*loader, "image/png");
  factory.register_by_mime_type(*loader, "image/x-png");

  factory.add_loader(std::move(loader));
}

} // namespace png
} // namespace surf

/* EOF */
