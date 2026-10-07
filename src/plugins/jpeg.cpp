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

#include <sstream>
#include <setjmp.h>

#include <geom/size.hpp>

#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"
#include "plugins/exif.hpp"
#include "plugins/file_jpeg_compressor.hpp"
#include "plugins/file_jpeg_decompressor.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/mem_jpeg_compressor.hpp"
#include "plugins/mem_jpeg_decompressor.hpp"
#include "transform.hpp"
#include "util/filesystem.hpp"

namespace surf {
namespace jpeg {

namespace {

#ifdef HAVE_EXIF

geom::isize apply_orientation(Transform modifier, const geom::isize& size)
{
  switch(modifier)
  {
    case Transform::ROTATE_90:
    case Transform::ROTATE_90_FLIP:
    case Transform::ROTATE_270:
    case Transform::ROTATE_270_FLIP:
      return geom::transpose(size);

    case Transform::ROTATE_0:
    case Transform::ROTATE_0_FLIP:
    case Transform::ROTATE_180:
    case Transform::ROTATE_180_FLIP:
    default:
      return size;
  }
}

#endif

} // namespace

bool filename_is_jpeg(std::filesystem::path const& filename)
{
  // FIXME: Merge this with util/jpeg_software_surface_loader, maybe
  // store the fileformat/SoftwareSurfaceLoader in the database intead
  // of figuring out the format each time anew
  return (Filesystem::get_extension(filename) == "jpg" ||
          Filesystem::get_extension(filename) == "jpeg");
}

geom::isize get_size(std::filesystem::path const& filename)
{
  FileJPEGDecompressor loader(filename);
  geom::isize size = loader.read_size();
#ifdef HAVE_EXIF
  return apply_orientation(exif::get_orientation(filename), size);
#else
  return size;
#endif
}


geom::isize get_size(std::span<uint8_t const> data)
{
  MemJPEGDecompressor loader(data);
  geom::isize size = loader.read_size();
#ifdef HAVE_EXIF
  return apply_orientation(exif::get_orientation(data), size);
#else
  return size;
#endif
}


SoftwareSurface load_from_file(std::filesystem::path const& filename, int scale, geom::isize* image_size)
{
  FileJPEGDecompressor loader(filename);
  PixelData<RGB8Pixel> surface = loader.read_image(scale, image_size);

#ifdef HAVE_EXIF
  Transform modifier = exif::get_orientation(filename);

  if (image_size) {
    *image_size = apply_orientation(modifier, *image_size);
  }

  if (modifier == Transform::ROTATE_0) {
    return SoftwareSurface{surface};
  } else {
    return SoftwareSurface{transform(surface, modifier)}; // FIXME: SLOW
  }
#else
  return SoftwareSurface{surface};
#endif
}


SoftwareSurface load_from_mem(std::span<uint8_t const> data, int scale, geom::isize* image_size)
{
  MemJPEGDecompressor loader(data);
  PixelData<RGB8Pixel> surface = loader.read_image(scale, image_size);

#ifdef HAVE_EXIF
  Transform modifier = exif::get_orientation(data);

  if (image_size) {
    *image_size = apply_orientation(modifier, *image_size);
  }

  if (modifier == Transform::ROTATE_0) {
    return SoftwareSurface(surface);
  } else {
    return SoftwareSurface(transform(surface, modifier)); // FIXME: SLOW;
  }
#else
  return SoftwareSurface(surface);
#endif
}

void save(SoftwareSurface const& surface, std::filesystem::path const& filename, int quality)
{
  FileJPEGCompressor compressor(filename);
  if (PixelView<RGB8Pixel> const* optional = surface.as_pixelview_ptr<RGB8Pixel>()) {
    compressor.save(*optional, quality);
  } else {
    compressor.save(convert(surface, surf::PixelFormat::RGB8).as_pixelview<RGB8Pixel>(), quality);
  }
}

std::vector<uint8_t> save(SoftwareSurface const& surface, int quality)
{
  std::vector<uint8_t> data;
  MemJPEGCompressor compressor(data);
  if (PixelView<RGB8Pixel> const* optional = surface.as_pixelview_ptr<RGB8Pixel>()) {
    compressor.save(*optional, quality);
  } else {
    compressor.save(convert(surface, surf::PixelFormat::RGB8).as_pixelview<RGB8Pixel>(), quality);
  }
  return data;
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("jpeg",
                            [](std::filesystem::path const& filename){ return load_from_file(filename); },
                            [](std::span<uint8_t const> data){ return load_from_mem(data); });

  factory.register_by_magic(*loader, "\xff\xd8");

  factory.register_by_mime_type(*loader, "image/jpeg");

  factory.register_by_extension(*loader, "jpeg");
  factory.register_by_extension(*loader, "jpg");

  factory.add_loader(std::move(loader));
}

} // namespace jpeg
} // namespace surf

/* EOF */
