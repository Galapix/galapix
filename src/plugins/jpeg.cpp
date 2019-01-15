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

#include <iostream>
#include <sstream>
#include <setjmp.h>

#include "math/size.hpp"
#include "plugins/exif.hpp"
#include "plugins/file_jpeg_compressor.hpp"
#include "plugins/file_jpeg_decompressor.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/mem_jpeg_compressor.hpp"
#include "plugins/mem_jpeg_decompressor.hpp"
#include "util/filesystem.hpp"

namespace {

Size apply_orientation(SoftwareSurface::Modifier modifier, const Size& size)
{
  switch(modifier)
  {
    case SoftwareSurface::kRot90:
    case SoftwareSurface::kRot90Flip:
    case SoftwareSurface::kRot270:
    case SoftwareSurface::kRot270Flip:
      return Size(size.height, size.width);

    case SoftwareSurface::kRot0:
    case SoftwareSurface::kRot0Flip:
    case SoftwareSurface::kRot180:
    case SoftwareSurface::kRot180Flip:
    default:
      return size;
  }
}

} // namespace


bool
JPEG::filename_is_jpeg(const std::string& filename)
{
  // FIXME: Merge this with util/jpeg_software_surface_loader, maybe
  // store the fileformat/SoftwareSurfaceLoader in the database intead
  // of figuring out the format each time anew
  return (Filesystem::get_extension(filename) == "jpg" ||
          Filesystem::get_extension(filename) == "jpeg");
}

Size
JPEG::get_size(const std::string& filename)
{
  FileJPEGDecompressor loader(filename);
  Size size = loader.read_size();
  return apply_orientation(EXIF::get_orientation(filename), size);
}


Size
JPEG::get_size(const uint8_t* data, int len)
{
  MemJPEGDecompressor loader(data, static_cast<size_t>(len));
  Size size = loader.read_size();
  return apply_orientation(EXIF::get_orientation(data, static_cast<size_t>(len)), size);
}


SoftwareSurface
JPEG::load_from_file(const std::string& filename, int scale, Size* image_size)
{
  FileJPEGDecompressor loader(filename);
  SoftwareSurface surface = loader.read_image(scale, image_size);

  SoftwareSurface::Modifier modifier = EXIF::get_orientation(filename);

  if (image_size) {
    *image_size = apply_orientation(modifier, *image_size);
  }

  if (modifier == SoftwareSurface::kRot0) {
    return surface;
  } else {
    return surface.transform(modifier);
  }
}


SoftwareSurface
JPEG::load_from_mem(const uint8_t* data, size_t len, int scale, Size* image_size)
{
  MemJPEGDecompressor loader(data, len);
  SoftwareSurface surface = loader.read_image(scale, image_size);

  SoftwareSurface::Modifier modifier = EXIF::get_orientation(data, len);

  if (image_size) {
    *image_size = apply_orientation(modifier, *image_size);
  }

  if (modifier == SoftwareSurface::kRot0) {
    return surface;
  } else {
    return surface.transform(modifier);
  }
}


void
JPEG::save(SoftwareSurface const& surface, int quality, const std::string& filename)
{
  FileJPEGCompressor compressor(filename);
  compressor.save(surface, quality);
}


Blob
JPEG::save(SoftwareSurface const& surface, int quality)
{
  std::vector<uint8_t> data;
  MemJPEGCompressor compressor(data);
  compressor.save(surface, quality);
  // FIXME: Unneeded copy of data
  return Blob::copy(data);
}

/* EOF */
