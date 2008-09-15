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

#include <assert.h>
#include <iostream>
#include <stdexcept>

#include "jpeg.hpp"
#include "png.hpp"
#include "imagemagick.hpp"
#include "xcf.hpp"

#include "blob.hpp"
#include "math.hpp"
#include "math/rect.hpp"
#include "math/rgb.hpp"
#include "math/rgba.hpp"
#include "math/size.hpp"

#include "filesystem.hpp"
#include "software_surface.hpp"

// FIXME: Stuff in this file is currently written to just work, not to
// be fast

SoftwareSurface::FileFormat
SoftwareSurface::get_fileformat(const URL& url)
{
  std::string filename = url.get_url();

  // FIXME: Make this more clever
  if (Filesystem::has_extension(filename, ".jpg")  ||
      Filesystem::has_extension(filename, ".JPG")  ||
      Filesystem::has_extension(filename, ".jpe")  ||
      Filesystem::has_extension(filename, ".JPE")  ||
      Filesystem::has_extension(filename, ".JPEG") ||
      Filesystem::has_extension(filename, ".jpeg"))
    {
      return JPEG_FILEFORMAT;
    }
  else if (Filesystem::has_extension(filename, ".PNG")  ||
           Filesystem::has_extension(filename, ".png"))
    {
      return PNG_FILEFORMAT;
    }
  else if (Filesystem::has_extension(filename, ".xcf") ||
           Filesystem::has_extension(filename, ".xcf.bz2") ||
           Filesystem::has_extension(filename, ".xcf.gz"))
    {
      return XCF_FILEFORMAT;
    }
  else if (Filesystem::has_extension(filename, ".gif") ||
           Filesystem::has_extension(filename, ".pnm") ||
           Filesystem::has_extension(filename, ".pgm") ||
           Filesystem::has_extension(filename, ".tif") ||
           Filesystem::has_extension(filename, ".TIF") ||
           Filesystem::has_extension(filename, ".tiff"))
    // FIXME: Add more stuff
    {
      return MAGICK_FILEFORMAT;
    }
  else
    {
      return UNKNOWN_FILEFORMAT;
    }
}

bool
SoftwareSurface::get_size(const URL& url, Size& size)
{
  switch(get_fileformat(url))
    {
      case JPEG_FILEFORMAT:
        return JPEG::get_size(url.get_stdio_name(), size);

      case PNG_FILEFORMAT:
        return PNG::get_size(url.get_stdio_name(), size);

      case XCF_FILEFORMAT:
        return XCF::get_size(url.get_stdio_name(), size);

      case MAGICK_FILEFORMAT:
        return Imagemagick::get_size(url.get_stdio_name(), size);

      default:
        return Imagemagick::get_size(url.get_stdio_name(), size);
    }
}

SoftwareSurface
SoftwareSurface::from_url(const URL& url)
{
  switch(get_fileformat(url))
    {
      case JPEG_FILEFORMAT:
        return JPEG::load_from_file(url.get_stdio_name());

      case PNG_FILEFORMAT:
        return PNG::load_from_file(url.get_stdio_name());

      case XCF_FILEFORMAT:
      case MAGICK_FILEFORMAT:
        return Imagemagick::load_from_file(url.get_stdio_name());

      default:
        throw std::runtime_error(url.get_url() + ": unknown file type");
        return SoftwareSurface();
    }  
}

class SoftwareSurfaceImpl
{
public:
  SoftwareSurface::Format format;
  Size     size;
  int      pitch;
  uint8_t* pixels;
  
  SoftwareSurfaceImpl(SoftwareSurface::Format format_, const Size& size_)
    : format(format_),
      size(size_)
  {
    switch(format)
      {
        case SoftwareSurface::RGB_FORMAT:
          pitch  = size.width * 3;
          pixels = new uint8_t[pitch * size.height];
          break;
          
        case SoftwareSurface::RGBA_FORMAT:
          pitch  = size.width * 4;
          pixels = new uint8_t[pitch * size.height];
          break;

        default:
          assert(!"SoftwareSurfaceImpl: Unknown color format");
      }
  }

  ~SoftwareSurfaceImpl() 
  {
    delete[] pixels;
  }
};

SoftwareSurface::SoftwareSurface()
{
}

SoftwareSurface::SoftwareSurface(Format format_, const Size& size_)
  : impl(new SoftwareSurfaceImpl(format_, size_))
{
}

SoftwareSurface::~SoftwareSurface()
{
}

void
SoftwareSurface::put_pixel(int x, int y, const RGBA& rgba)
{
  assert(impl->format == RGBA_FORMAT);
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  impl->pixels[y * impl->pitch + x*4 + 0] = rgba.r;
  impl->pixels[y * impl->pitch + x*4 + 1] = rgba.g;
  impl->pixels[y * impl->pitch + x*4 + 2] = rgba.b;  
  impl->pixels[y * impl->pitch + x*4 + 3] = rgba.a;
}

void
SoftwareSurface::get_pixel(int x, int y, RGBA& rgb) const
{
  assert(impl->format == RGBA_FORMAT);
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);
  
  rgb.r = impl->pixels[y * impl->pitch + x*4 + 0];
  rgb.g = impl->pixels[y * impl->pitch + x*4 + 1];
  rgb.b = impl->pixels[y * impl->pitch + x*4 + 2];
  rgb.a = impl->pixels[y * impl->pitch + x*4 + 3];
}

void
SoftwareSurface::put_pixel(int x, int y, const RGB& rgb)
{
  assert(impl->format == RGB_FORMAT);
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  impl->pixels[y * impl->pitch + x*3 + 0] = rgb.r;
  impl->pixels[y * impl->pitch + x*3 + 1] = rgb.g;
  impl->pixels[y * impl->pitch + x*3 + 2] = rgb.b;
}

void
SoftwareSurface::get_pixel(int x, int y, RGB& rgb) const
{
  assert(impl->format == RGB_FORMAT);
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  rgb.r = impl->pixels[y * impl->pitch + x*3 + 0];
  rgb.g = impl->pixels[y * impl->pitch + x*3 + 1];
  rgb.b = impl->pixels[y * impl->pitch + x*3 + 2];
}

SoftwareSurface
SoftwareSurface::halve() const
{
  return scale(Size(get_width()/2, get_height()/2));
}

SoftwareSurface
SoftwareSurface::scale(const Size& size) const
{
  SoftwareSurface surface(impl->format, size);
  // FIXME: very much non-fast, needs replacement with proper

  switch(impl->format)
    {
      case RGB_FORMAT:
        {
          RGB rgb;
          for(int y = 0; y < surface.get_height(); ++y)
            for(int x = 0; x < surface.get_width(); ++x)
              {
                get_pixel(x * impl->size.width  / surface.impl->size.width,
                          y * impl->size.height / surface.impl->size.height,
                          rgb);
                
                surface.put_pixel(x, y, rgb);
              }
        }
        break;

      case RGBA_FORMAT:
        {
          RGBA rgba;
          for(int y = 0; y < surface.get_height(); ++y)
            for(int x = 0; x < surface.get_width(); ++x)
              {
                get_pixel(x * impl->size.width  / surface.impl->size.width,
                          y * impl->size.height / surface.impl->size.height,
                          rgba);
                
                surface.put_pixel(x, y, rgba);
              }
        }
        break;

      default:
        assert(!"SoftwareSurface::scale: Unknown format");
        break;
    }

  return surface;
}

SoftwareSurface
SoftwareSurface::crop(const Rect& rect_in) const
{
  // FIXME: We could do a crop without copying contain, simply
  // reference the old SoftwareSurfaceImpl and have a different pitch
  // and pixel offset
  assert(rect_in.is_normal());
 
  // Clip the rectangle to the image
  Rect rect(Math::clamp(0, rect_in.left,   get_width()),
            Math::clamp(0, rect_in.top,    get_height()),
            Math::clamp(0, rect_in.right,  get_width()), 
            Math::clamp(0, rect_in.bottom, get_height()));

  SoftwareSurface surface(impl->format, rect.get_size());

  for(int y = rect.top; y < rect.bottom; ++y)
    {
      memcpy(surface.get_row_data(y - rect.top), 
             get_row_data(y) + rect.left * get_bytes_per_pixel(),
             rect.get_width() * get_bytes_per_pixel());
    }

  return surface;
}

Size
SoftwareSurface::get_size()  const
{
  return impl->size;
}

int
SoftwareSurface::get_width()  const
{
  return impl->size.width;
}

int
SoftwareSurface::get_height() const
{
  return impl->size.height;
}

int
SoftwareSurface::get_pitch()  const
{
  return impl->pitch;
}

void
SoftwareSurface::save(const std::string& filename) const
{
  assert(!"SoftwareSurface::save(const std::string& filename) const");
}

Blob
SoftwareSurface::get_raw_data() const
{
  assert(impl->pitch != impl->size.width*3);
  return Blob(impl->pixels, impl->size.height * impl->pitch);
}

uint8_t*
SoftwareSurface::get_data() const
{
  return impl->pixels;
}

uint8_t*
SoftwareSurface::get_row_data(int y) const
{
  return impl->pixels + (y * impl->pitch);
  
}

SoftwareSurface::Format
SoftwareSurface::get_format() const
{
  return impl->format;
}

RGB
SoftwareSurface::get_average_color() const
{
  assert(impl->format == RGB_FORMAT);
  // Only works for smaller surfaces, else we would run into integer overflows
  assert(get_width() > 256 || get_height() > 256); // random limit, but should be enough for galapix

  unsigned int r = 0;
  unsigned int g = 0;
  unsigned int b = 0;

  for(int y = 0; y < get_height(); ++y)
    for(int x = 0; x < get_width(); ++x)
      {
        RGB rgb;
        get_pixel(x, y, rgb);

        r += rgb.r;
        g += rgb.g;
        b += rgb.b;
      }

  int num_pixels = get_width() * get_height();
  return RGB(r / num_pixels,
             g / num_pixels,
             b / num_pixels);
}

SoftwareSurface
SoftwareSurface::to_rgb() const
{
  switch(impl->format)
    {
      case RGB_FORMAT:
        return *this;
        
      case RGBA_FORMAT:
        {
          SoftwareSurface surface(RGB_FORMAT, impl->size);

          int num_pixels      = get_width() * get_height();
          uint8_t* src_pixels = get_data();
          uint8_t* dst_pixels = surface.get_data();

          for(int i = 0; i < num_pixels; ++i)
            {
              dst_pixels[3*i+0] = src_pixels[4*i+0];
              dst_pixels[3*i+1] = src_pixels[4*i+1];
              dst_pixels[3*i+2] = src_pixels[4*i+2];
            }

          return surface;
        }
        
      default:
        assert(!"SoftwareSurface::to_rgb: Unknown format");
        return SoftwareSurface();
    }
}

int
SoftwareSurface::get_bytes_per_pixel() const
{
  switch(impl->format) 
    {
      case RGB_FORMAT:
        return 3;

      case RGBA_FORMAT:
        return 4;

      default:
        assert(!"SoftwareSurface::get_bytes_per_pixel(): Unknown format");
    }
}
  
/* EOF */
