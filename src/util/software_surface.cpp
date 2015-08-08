/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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
#include <boost/scoped_array.hpp>

#include "math/rect.hpp"
#include "math/rgb.hpp"
#include "math/rgba.hpp"

// FIXME: Stuff in this file is currently written to just work, not to
// be fast

namespace {

inline
void copy_pixel_rgb(SoftwareSurface& dst, int dst_x, int dst_y,
                    SoftwareSurface& src, int src_x, int src_y)
{
  uint8_t* const d = dst.get_row_data(dst_y) + 3*dst_x;
  uint8_t* const s = src.get_row_data(src_y) + 3*src_x;
  d[0] = s[0];
  d[1] = s[1];
  d[2] = s[2];
}

inline
void copy_pixel_rgba(SoftwareSurface& dst, int dst_x, int dst_y,
                     SoftwareSurface& src, int src_x, int src_y)
{
  uint32_t* const d = reinterpret_cast<uint32_t*>(dst.get_row_data(dst_y) + 4*dst_x);
  uint32_t* const s = reinterpret_cast<uint32_t*>(src.get_row_data(src_y) + 4*src_x);
  *d = *s;
}

} // namespace

class SoftwareSurfaceImpl
{
public:
  SoftwareSurface::Format format;
  Size     size;
  int      pitch;
  std::unique_ptr<uint8_t[]> pixels;

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
        pixels.reset(new uint8_t[static_cast<size_t>(pitch * size.height)]);
        break;

      case SoftwareSurface::RGBA_FORMAT:
        pitch  = size.width * 4;
        pixels.reset(new uint8_t[static_cast<size_t>(pitch * size.height)]);
        break;

      default:
        assert(false && "SoftwareSurfaceImpl: Unknown color format");
    }
  }
};

SoftwareSurfacePtr
SoftwareSurface::create(Format format, const Size& size)
{
  return SoftwareSurfacePtr(new SoftwareSurface(format, size));
}

SoftwareSurface::SoftwareSurface(Format format_, const Size& size_) :
  impl(new SoftwareSurfaceImpl(format_, size_))
{
}

void
SoftwareSurface::put_pixel(int x, int y, const RGBA& rgba)
{
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  switch(impl->format)
  {
    case RGBA_FORMAT:
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 0)] = rgba.r;
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 1)] = rgba.g;
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 2)] = rgba.b;
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 3)] = rgba.a;
      break;

    case RGB_FORMAT:
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 0)] = rgba.r;
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 1)] = rgba.g;
      impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 2)] = rgba.b;
      break;
  }
}

void
SoftwareSurface::get_pixel(int x, int y, RGBA& rgb) const
{
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  switch(impl->format)
  {
    case RGBA_FORMAT:
      rgb.r = impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 0)];
      rgb.g = impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 1)];
      rgb.b = impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 2)];
      rgb.a = impl->pixels[static_cast<size_t>(y * impl->pitch + x*4 + 3)];
      break;

    case RGB_FORMAT:
      rgb.r = impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 0)];
      rgb.g = impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 1)];
      rgb.b = impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 2)];
      rgb.a = 255;
      break;
  }
}

void
SoftwareSurface::put_pixel(int x, int y, const RGB& rgb)
{
  assert(impl->format == RGB_FORMAT);
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 0)] = rgb.r;
  impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 1)] = rgb.g;
  impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 2)] = rgb.b;
}

void
SoftwareSurface::get_pixel(int x, int y, RGB& rgb) const
{
  assert(impl->format == RGB_FORMAT);
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  rgb.r = impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 0)];
  rgb.g = impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 1)];
  rgb.b = impl->pixels[static_cast<size_t>(y * impl->pitch + x*3 + 2)];
}

SoftwareSurfacePtr
SoftwareSurface::halve()
{
  SoftwareSurfacePtr dstsrc = SoftwareSurface::create(impl->format, impl->size/2);

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
      assert(false && "Not reachable");
      break;
  }

  return dstsrc;
}

SoftwareSurfacePtr
SoftwareSurface::scale(const Size& size)
{
  if (size == impl->size)
  {
    return clone();
  }
  else
  {
    SoftwareSurfacePtr surface = SoftwareSurface::create(impl->format, size);
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
        assert(false && "SoftwareSurface::scale: Unknown format");
        break;
    }

    return surface;
  }
}

SoftwareSurfacePtr
SoftwareSurface::clone()
{
  SoftwareSurfacePtr out = SoftwareSurface::create(impl->format, impl->size);
  memcpy(out->impl->pixels.get(),
         impl->pixels.get(),
         static_cast<size_t>(impl->pitch * impl->size.height));
  return out;
}

SoftwareSurfacePtr
SoftwareSurface::transform(Modifier mod)
{
  switch(mod)
  {
    case kRot0:
      return clone();

    case kRot90:
      return rotate90();

    case kRot180:
      return rotate180();

    case kRot270:
      return rotate270();

    case kRot0Flip:
      return vflip();

    case kRot90Flip:
      // FIXME: Could be made faster
      return rotate90()->vflip();

    case kRot180Flip:
      return hflip();

    case kRot270Flip:
      // FIXME: Could be made faster
      return rotate270()->vflip();

    default:
      assert(false && "never reached");
      return clone();
  }
}

SoftwareSurfacePtr
SoftwareSurface::rotate90()
{
  SoftwareSurfacePtr out = SoftwareSurface::create(impl->format, Size(impl->size.height, impl->size.width));

  switch(impl->format)
  {
    case SoftwareSurface::RGB_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgb(*out, impl->size.height - y - 1, x,
                         *this, x, y);
        }
      break;

    case SoftwareSurface::RGBA_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
      {
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgba(*out, impl->size.height - y - 1, x,
                          *this, x, y);
        }
      }
      break;
  }

  return out;
}

SoftwareSurfacePtr
SoftwareSurface::rotate180()
{
  SoftwareSurfacePtr out = SoftwareSurface::create(impl->format, impl->size);

  switch(impl->format)
  {
    case SoftwareSurface::RGB_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgb(*out, impl->size.width - x - 1, impl->size.height - 1 - y,
                         *this, x, y);
        }
      break;

    case SoftwareSurface::RGBA_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
      {
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgba(*out, impl->size.width - x - 1, impl->size.height - 1 - y,
                          *this, x, y);
        }
      }
      break;
  }

  return out;
}

SoftwareSurfacePtr
SoftwareSurface::rotate270()
{
  SoftwareSurfacePtr out = SoftwareSurface::create(impl->format, Size(impl->size.height, impl->size.width));

  switch(impl->format)
  {
    case SoftwareSurface::RGB_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgb(*out, y, impl->size.width - 1 - x,
                         *this, x, y);
        }
      break;

    case SoftwareSurface::RGBA_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
      {
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgba(*out, y, impl->size.width - 1 - x,
                          *this, x, y);
        }
      }
      break;
  }

  return out;
}

SoftwareSurfacePtr
SoftwareSurface::hflip()
{
  SoftwareSurfacePtr out = SoftwareSurface::create(impl->format, impl->size);

  switch(impl->format)
  {
    case SoftwareSurface::RGB_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgb(*out, impl->size.width - 1 - x, y,
                         *this, x, y);
        }
      break;

    case SoftwareSurface::RGBA_FORMAT:
      for(int y = 0; y < impl->size.height; ++y)
      {
        for(int x = 0; x < impl->size.width; ++x)
        {
          copy_pixel_rgba(*out, impl->size.width - 1 - x, y,
                          *this, x, y);
        }
      }
      break;
  }

  return out;
}

SoftwareSurfacePtr
SoftwareSurface::vflip()
{
  SoftwareSurfacePtr out = SoftwareSurface::create(impl->format, impl->size);

  for(int y = 0; y < impl->size.height; ++y)
  {
    memcpy(out->get_row_data(impl->size.height - y - 1),
           get_row_data(y),
           static_cast<size_t>(impl->pitch));
  }

  return out;
}

SoftwareSurfacePtr
SoftwareSurface::crop(const Rect& rect_in)
{
  // FIXME: We could do a crop without copying content, simply
  // reference the old SoftwareSurfaceImpl and have a different pitch
  // and pixel offset
  assert(rect_in.is_normal());

  // Clip the rectangle to the image
  Rect rect(Math::clamp(0, rect_in.left,   get_width()),
            Math::clamp(0, rect_in.top,    get_height()),
            Math::clamp(0, rect_in.right,  get_width()),
            Math::clamp(0, rect_in.bottom, get_height()));

  SoftwareSurfacePtr surface = SoftwareSurface::create(impl->format, rect.get_size());

  for(int y = rect.top; y < rect.bottom; ++y)
  {
    memcpy(surface->get_row_data(y - rect.top),
           get_row_data(y) + rect.left * get_bytes_per_pixel(),
           static_cast<size_t>(rect.get_width() * get_bytes_per_pixel()));
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

Blob
SoftwareSurface::get_raw_data() const
{
  assert(impl->pitch != impl->size.width*3);
  return Blob::copy(impl->pixels.get(),
                    static_cast<size_t>(impl->size.height * impl->pitch));
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

  unsigned int num_pixels = static_cast<unsigned int>(get_width() * get_height());
  return RGB(static_cast<uint8_t>(r / num_pixels),
             static_cast<uint8_t>(g / num_pixels),
             static_cast<uint8_t>(b / num_pixels));
}

SoftwareSurfacePtr
SoftwareSurface::to_rgb()
{
  switch(impl->format)
  {
    case RGB_FORMAT:
      return clone();

    case RGBA_FORMAT:
    {
      SoftwareSurfacePtr surface = SoftwareSurface::create(RGB_FORMAT, impl->size);

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
      assert(false && "SoftwareSurface::to_rgb: Unknown format");
      return {};
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
      assert(false && "SoftwareSurface::get_bytes_per_pixel(): Unknown format");
      return 0;
  }
}

void
SoftwareSurface::blit(SoftwareSurfacePtr& dst, const Vector2i& pos)
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
             static_cast<size_t>((end_x - start_x) * 3));
  }
  else if (dst->impl->format == RGBA_FORMAT && impl->format == RGBA_FORMAT)
  {
    for(int y = start_y; y < end_y; ++y)
      memcpy(dst->get_row_data(y + pos.y) + (pos.x+start_x)*4,
             get_row_data(y) + start_x*4,
             static_cast<size_t>((end_x - start_x) * 4));
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
    assert(false && "Not implemented");
  }
}

/* EOF */
