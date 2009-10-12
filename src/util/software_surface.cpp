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

#include "util/software_surface.hpp"

#include <iostream>
#include <string.h>

#include "math/rect.hpp"
#include "math/rgb.hpp"
#include "math/rgba.hpp"

// FIXME: Stuff in this file is currently written to just work, not to
// be fast

class SoftwareSurfaceImpl
{
public:
  SoftwareSurface::Format format;
  Size     size;
  int      pitch;
  boost::scoped_array<uint8_t> pixels;
  
  SoftwareSurfaceImpl(SoftwareSurface::Format format_, const Size& size_) :
    format(format_),
    size(size_),
    pitch(),
    pixels()
  {
    switch(format)
    {
      case SoftwareSurface::RGB_FORMAT:
        pitch  = size.width * 3;
        pixels.reset(new uint8_t[pitch * size.height]);
        break;
          
      case SoftwareSurface::RGBA_FORMAT:
        pitch  = size.width * 4;
        pixels.reset(new uint8_t[pitch * size.height]);
        break;

      default:
        assert(!"SoftwareSurfaceImpl: Unknown color format");
    }
  }
};

SoftwareSurfaceHandle
SoftwareSurface::create(Format format, const Size& size)
{
  return SoftwareSurfaceHandle(new SoftwareSurface(format, size));
}

SoftwareSurface::SoftwareSurface(Format format_, const Size& size_) :
  impl(new SoftwareSurfaceImpl(format_, size_))
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

SoftwareSurfaceHandle
SoftwareSurface::halve()
{
  SoftwareSurfaceHandle dstsrc = SoftwareSurface::create(impl->format, impl->size/2);
  
  uint8_t* dst = dstsrc->get_data();
  uint8_t* src = get_data();

  //int src_w = get_width();
  //int src_h = get_height();
  int src_p = get_pitch();

  int dst_w = dstsrc->get_width();
  int dst_h = dstsrc->get_height();
  int dst_p = dstsrc->get_pitch();

  switch(impl->format)
  {
    case RGB_FORMAT:
      for(int y = 0; y < dst_h; ++y)
        for(int x = 0; x < dst_w; ++x)
        {
          uint8_t* d = dst + (y*dst_p + 3*x);
          uint8_t* s = src + (y*src_p + 3*x)*2;

          d[0] = static_cast<uint8_t>((s[0] + s[0+3] + s[0+src_p] + s[0+src_p+3])/4);
          d[1] = static_cast<uint8_t>((s[1] + s[1+3] + s[1+src_p] + s[1+src_p+3])/4);
          d[2] = static_cast<uint8_t>((s[2] + s[2+3] + s[2+src_p] + s[2+src_p+3])/4);
        }
      break;

    case RGBA_FORMAT:
      for(int y = 0; y < dst_h; ++y)
        for(int x = 0; x < dst_w; ++x)
        {
          uint8_t* d = dst + (y*dst_p + 4*x);
          uint8_t* s = src + (y*src_p + 4*x)*2;

          d[0] = static_cast<uint8_t>((s[0] + s[0+4] + s[0+src_p] + s[0+src_p+4])/4);
          d[1] = static_cast<uint8_t>((s[1] + s[1+4] + s[1+src_p] + s[1+src_p+4])/4);
          d[2] = static_cast<uint8_t>((s[2] + s[2+4] + s[2+src_p] + s[2+src_p+4])/4);
          d[3] = static_cast<uint8_t>((s[3] + s[3+4] + s[3+src_p] + s[3+src_p+4])/4);
        }
      break;
        
    default:
      assert(!"Not reachable");
      break;
  }

  return dstsrc;
}

SoftwareSurfaceHandle
SoftwareSurface::scale(const Size& size)
{
  if (size == impl->size)
  {
    return shared_from_this();
  }
  else
  {
    SoftwareSurfaceHandle surface = SoftwareSurface::create(impl->format, size);
    // FIXME: very much non-fast
    switch(impl->format)
    {
      case RGB_FORMAT:
      {
        RGB rgb;
        for(int y = 0; y < surface->get_height(); ++y)
          for(int x = 0; x < surface->get_width(); ++x)
          {
            get_pixel(x * impl->size.width  / surface->impl->size.width,
                      y * impl->size.height / surface->impl->size.height,
                      rgb);
                
            surface->put_pixel(x, y, rgb);
          }
      }
      break;

      case RGBA_FORMAT:
      {
        RGBA rgba;
        for(int y = 0; y < surface->get_height(); ++y)
          for(int x = 0; x < surface->get_width(); ++x)
          {
            get_pixel(x * impl->size.width  / surface->impl->size.width,
                      y * impl->size.height / surface->impl->size.height,
                      rgba);
                
            surface->put_pixel(x, y, rgba);
          }
      }
      break;

      default:
        assert(!"SoftwareSurface::scale: Unknown format");
        break;
    }

    return surface;
  }
}

SoftwareSurfaceHandle
SoftwareSurface::vflip()
{
  SoftwareSurfaceHandle out = SoftwareSurface::create(impl->format, impl->size);

  for(int y = 0; y < impl->size.height; ++y)
    memcpy(out->get_row_data(impl->size.height - y - 1), get_row_data(y), impl->pitch);

  return out;
}

SoftwareSurfaceHandle
SoftwareSurface::crop(const Rect& rect_in)
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

  SoftwareSurfaceHandle surface = SoftwareSurface::create(impl->format, rect.get_size());

  for(int y = rect.top; y < rect.bottom; ++y)
  {
    memcpy(surface->get_row_data(y - rect.top), 
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

BlobHandle
SoftwareSurface::get_raw_data() const
{
  assert(impl->pitch != impl->size.width*3);
  return Blob::copy(impl->pixels.get(), impl->size.height * impl->pitch);
}

uint8_t*
SoftwareSurface::get_data() const
{
  return impl->pixels.get();
}

uint8_t*
SoftwareSurface::get_row_data(int y) const
{
  return impl->pixels.get() + (y * impl->pitch);
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
  return RGB(static_cast<uint8_t>(r / num_pixels),
             static_cast<uint8_t>(g / num_pixels),
             static_cast<uint8_t>(b / num_pixels));
}

SoftwareSurfaceHandle
SoftwareSurface::to_rgb()
{
  switch(impl->format)
  {
    case RGB_FORMAT:
      return shared_from_this();
        
    case RGBA_FORMAT:
    {
      SoftwareSurfaceHandle surface = SoftwareSurface::create(RGB_FORMAT, impl->size);

      int num_pixels      = get_width() * get_height();
      uint8_t* src_pixels = get_data();
      uint8_t* dst_pixels = surface->get_data();

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
      return SoftwareSurfaceHandle();
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
      return 0;
  }
}

void
SoftwareSurface::blit(SoftwareSurfaceHandle& dst, const Vector2i& pos)
{
  int start_x = std::max(0, -pos.x);
  int start_y = std::max(0, -pos.y);

  int end_x = std::min(impl->size.width,  dst->impl->size.width  - pos.x);
  int end_y = std::min(impl->size.height, dst->impl->size.height - pos.y);

  if (dst->impl->format == RGB_FORMAT && impl->format == RGB_FORMAT)
  {
    for(int y = start_y; y < end_y; ++y)
      memcpy(dst->get_row_data(y + pos.y) + (pos.x+start_x)*3, 
             get_row_data(y) + start_x*3,
             (end_x - start_x)*3);
  }
  else if (dst->impl->format == RGBA_FORMAT && impl->format == RGBA_FORMAT)
  {
    for(int y = start_y; y < end_y; ++y)
      memcpy(dst->get_row_data(y + pos.y) + (pos.x+start_x)*4, 
             get_row_data(y) + start_x*4,
             (end_x - start_x)*4);
  }
  else if (dst->impl->format == RGBA_FORMAT && impl->format == RGB_FORMAT)
  {
    for(int y = start_y; y < end_y; ++y)
    {
      uint8_t* dstpx = dst->get_row_data(y + pos.y) + (pos.x+start_x)*4;
      uint8_t* srcpx = get_row_data(y) + start_x*3;
          
      for(int x = 0; x < (end_x - start_x); ++x)
      { 
        dstpx[4*x+0] = srcpx[3*x+0];
        dstpx[4*x+1] = srcpx[3*x+1];
        dstpx[4*x+2] = srcpx[3*x+2];
        dstpx[4*x+3] = 255;
      }
    }
  }
  else if (dst->impl->format == RGB_FORMAT && impl->format == RGBA_FORMAT)
  {
    for(int y = start_y; y < end_y; ++y)
    {
      uint8_t* dstpx = dst->get_row_data(y + pos.y) + (pos.x+start_x)*3;
      uint8_t* srcpx = get_row_data(y) + start_x*4;
          
      for(int x = 0; x < (end_x - start_x); ++x)
      { 
        uint8_t alpha = srcpx[4*x+3];
        dstpx[3*x+0] = static_cast<uint8_t>(srcpx[4*x+0] * alpha / 255);
        dstpx[3*x+1] = static_cast<uint8_t>(srcpx[4*x+1] * alpha / 255);
        dstpx[3*x+2] = static_cast<uint8_t>(srcpx[4*x+2] * alpha / 255);
      }
    }
  }
  else
  {
    assert(!"Not implemented");
  }
}
  
/* EOF */
