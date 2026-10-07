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

#include "plugins/imagemagick.hpp"

#ifdef HAVE_MAGICKXX

#include <algorithm>
#include <assert.h>
#include <list>
#include <stdexcept>

#include <logmich/log.hpp>
#include <Magick++.h>

#include <geom/size.hpp>

#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"

namespace surf {
namespace imagemagick {

namespace {

SoftwareSurface
MagickImage2PixelData(const Magick::Image& image)
{
  int width  = static_cast<int>(image.columns());
  int height = static_cast<int>(image.rows());
  int shift = QuantumDepth - 8;

  if (image.matte())
  {
    PixelData<RGBAPixel> dst(geom::isize(width, height));

    for(int y = 0; y < height; ++y)
    {
      const Magick::PixelPacket* src_pixels = image.getConstPixels(0, y, static_cast<size_t>(width), 1);
      RGBAPixel* dst_pixels = dst.get_row(y);

      for(int x = 0; x < width; ++x)
      {
        dst_pixels[x] = RGBAPixel{
          static_cast<uint8_t>(src_pixels[x].red   >> shift),
          static_cast<uint8_t>(src_pixels[x].green >> shift),
          static_cast<uint8_t>(src_pixels[x].blue  >> shift),
          static_cast<uint8_t>(255 - (src_pixels[x].opacity >> shift))
        };
      }
    }

    return SoftwareSurface(dst);
  }
  else
  {
    PixelData<RGBPixel> dst(geom::isize(width, height));

    for(int y = 0; y < height; ++y)
    {
      RGBPixel* dst_pixels = dst.get_row(y);
      const Magick::PixelPacket* src_pixels = image.getConstPixels(0, y, static_cast<size_t>(width), 1);

      for(int x = 0; x < width; ++x)
      {
        dst_pixels[x] = RGBPixel{
          static_cast<uint8_t>(src_pixels[x].red   >> shift),
          static_cast<uint8_t>(src_pixels[x].green >> shift),
          static_cast<uint8_t>(src_pixels[x].blue  >> shift)
        };
      }
    }

    return SoftwareSurface(dst);
  }
}

} // namespace

bool get_size(std::filesystem::path const& filename, geom::isize& size)
{
  try
  {
    Magick::Image image(filename);

    size = geom::isize(static_cast<int>(image.columns()),
                static_cast<int>(image.rows()));

    return true;
  }
  catch(std::exception& err)
  {
    log_error("Imagemagick: {}: {}", filename.string(), err.what());
    return false;
  }
}

std::vector<std::string> get_supported_extensions()
{
  if ((false)) // FIXME: disabled for reasons
  {
    /* Generating the list automatic doesn't work, as there ends up to
       be to much weird stuff in it (txt, avi, mpeg, etc.) that causes
       trouble */
    std::list<Magick::CoderInfo> coderList;

    Magick::coderInfoList(&coderList,
                          Magick::CoderInfo::TrueMatch, // Match readable formats
                          Magick::CoderInfo::AnyMatch,  // Don't care about writable formats
                          Magick::CoderInfo::AnyMatch); // Don't care about multi-frame support

    std::vector<std::string> lst;

    for(std::list<Magick::CoderInfo>::iterator entry = coderList.begin();
        entry != coderList.end();
        ++entry)
    {
      std::string data = entry->name();
      std::transform(data.begin(), data.end(), data.begin(), ::tolower);
      lst.push_back(data);
    }

    return lst;
  }
  else
  {
    std::vector<std::string> lst;

    lst.push_back("gif");
    lst.push_back("tga");
    lst.push_back("rgb");
    lst.push_back("bmp");
    lst.push_back("tiff");
    lst.push_back("tif");
    lst.push_back("pbm");
    lst.push_back("ppm");
    lst.push_back("pgm");

    return lst;
  }
}

SoftwareSurface load_from_mem(std::span<uint8_t const> data)
{
  // FIXME: Magick::Blob creates an unneeded copy of the data
  return MagickImage2PixelData(Magick::Image(Magick::Blob(data.data(), data.size())));
}

SoftwareSurface load_from_file(std::filesystem::path const& filename)
{
  return MagickImage2PixelData(Magick::Image(filename));
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("imagemagick", load_from_file, load_from_mem);
    std::vector<std::string> lst = imagemagick::get_supported_extensions();
    for(std::vector<std::string>::const_iterator i = lst.begin();
        i != lst.end(); ++i)
    {
      factory.register_by_extension(*loader, *i);
    }
  factory.add_loader(std::move(loader));
}

} // namespace imagemagick
} // namespace surf

#endif

/* EOF */
