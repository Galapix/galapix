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

namespace {

inline
void copy_pixel_rgb(PixelData& dst, int dst_x, int dst_y,
                    PixelData const& src, int src_x, int src_y)
{
  uint8_t* const d = dst.get_row_data(dst_y) + 3*dst_x;
  uint8_t const* const s = src.get_row_data(src_y) + 3*src_x;
  d[0] = s[0];
  d[1] = s[1];
  d[2] = s[2];
}

inline
void copy_pixel_rgba(PixelData& dst, int dst_x, int dst_y,
                     PixelData const& src, int src_x, int src_y)
{
  uint32_t* const d = reinterpret_cast<uint32_t*>(dst.get_row_data(dst_y) + 4*dst_x);
  uint32_t const* const s = reinterpret_cast<uint32_t const*>(src.get_row_data(src_y) + 4*src_x);
  *d = *s;
}

} // namespace

SoftwareSurface::SoftwareSurface() :
  m_pixel_data()
{
}

SoftwareSurface::SoftwareSurface(PixelData data) :
  m_pixel_data(std::make_shared<PixelData>(std::move(data)))
{
}

SoftwareSurface
SoftwareSurface::halve() const
{
  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), src.get_size() / 2);

  int src_p = src.get_pitch();

  int dst_w = dst.get_width();
  int dst_h = dst.get_height();
  int dst_p = dst.get_pitch();

  switch(src.get_format())
  {
    case PixelData::RGB_FORMAT:
      for(int y = 0; y < dst_h; ++y)
        for(int x = 0; x < dst_w; ++x)
        {
          uint8_t* d = dst.get_data() + (y*dst_p + 3*x);
          uint8_t const* s = src.get_data() + (y*src_p + 3*x)*2;

          d[0] = static_cast<uint8_t>((s[0] + s[0+3] + s[0+src_p] + s[0+src_p+3])/4);
          d[1] = static_cast<uint8_t>((s[1] + s[1+3] + s[1+src_p] + s[1+src_p+3])/4);
          d[2] = static_cast<uint8_t>((s[2] + s[2+3] + s[2+src_p] + s[2+src_p+3])/4);
        }
      break;

    case PixelData::RGBA_FORMAT:
      for(int y = 0; y < dst_h; ++y)
        for(int x = 0; x < dst_w; ++x)
        {
          uint8_t* d = dst.get_data() + (y*dst_p + 4*x);
          uint8_t const* s = src.get_data() + (y*src_p + 4*x)*2;

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

  return SoftwareSurface(std::move(dst));
}

SoftwareSurface
SoftwareSurface::scale(Size const& size) const
{
  PixelData const& src = *m_pixel_data;

  if (size == src.get_size())
  {
    return *this;
  }
  else
  {
    PixelData dst(src.get_format(), size);

    // FIXME: very much non-fast
    switch(src.get_format())
    {
      case PixelData::RGB_FORMAT:
      {
        RGB rgb;
        for(int y = 0; y < dst.get_height(); ++y)
          for(int x = 0; x < dst.get_width(); ++x)
          {
            get_pixel(x * src.get_size().width  / dst.get_size().width,
                      y * src.get_size().height / dst.get_size().height,
                      rgb);

            dst.put_pixel(x, y, rgb);
          }
      }
      break;

      case PixelData::RGBA_FORMAT:
      {
        RGBA rgba;
        for(int y = 0; y < dst.get_height(); ++y)
          for(int x = 0; x < dst.get_width(); ++x)
          {
            get_pixel(x * src.get_size().width  / dst.get_size().width,
                      y * src.get_size().height / dst.get_size().height,
                      rgba);

            dst.put_pixel(x, y, rgba);
          }
      }
      break;

      default:
        assert(false && "SoftwareSurface::scale: Unknown format");
        break;
    }

    return SoftwareSurface(std::move(dst));
  }
}

SoftwareSurface
SoftwareSurface::transform(Modifier mod) const
{
  switch(mod)
  {
    case kRot0:
      return *this;

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
      return rotate90().vflip();

    case kRot180Flip:
      return hflip();

    case kRot270Flip:
      // FIXME: Could be made faster
      return rotate270().vflip();

    default:
      assert(false && "never reached");
      return *this;
  }
}

SoftwareSurface
SoftwareSurface::rotate90() const
{
  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), Size(src.get_size().height, src.get_size().width));

  switch(src.get_format())
  {
    case PixelData::RGB_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgb(dst, src.get_size().height - y - 1, x,
                         src, x, y);
        }
      break;

    case PixelData::RGBA_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
      {
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgba(dst, src.get_size().height - y - 1, x,
                          src, x, y);
        }
      }
      break;
  }

  return SoftwareSurface(std::move(dst));
}

SoftwareSurface
SoftwareSurface::rotate180() const
{
  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), src.get_size());

  switch(src.get_format())
  {
    case PixelData::RGB_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgb(dst, src.get_size().width - x - 1, src.get_size().height - 1 - y,
                         src, x, y);
        }
      break;

    case PixelData::RGBA_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
      {
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgba(dst, src.get_size().width - x - 1, src.get_size().height - 1 - y,
                          src, x, y);
        }
      }
      break;
  }

  return SoftwareSurface(std::move(dst));
}

SoftwareSurface
SoftwareSurface::rotate270() const
{
  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), Size(src.get_height(), src.get_width()));

  switch(src.get_format())
  {
    case PixelData::RGB_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgb(dst, y, src.get_size().width - 1 - x,
                         src, x, y);
        }
      break;

    case PixelData::RGBA_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
      {
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgba(dst, y, src.get_size().width - 1 - x,
                          src, x, y);
        }
      }
      break;
  }

  return SoftwareSurface(std::move(dst));
}

SoftwareSurface
SoftwareSurface::hflip() const
{
  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), src.get_size());

  switch(src.get_format())
  {
    case PixelData::RGB_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgb(dst, src.get_size().width - 1 - x, y,
                         src, x, y);
        }
      break;

    case PixelData::RGBA_FORMAT:
      for(int y = 0; y < src.get_size().height; ++y)
      {
        for(int x = 0; x < src.get_size().width; ++x)
        {
          copy_pixel_rgba(dst, src.get_size().width - 1 - x, y,
                          src, x, y);
        }
      }
      break;
  }

  return SoftwareSurface(std::move(dst));
}

SoftwareSurface
SoftwareSurface::vflip() const
{
  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), src.get_size());

  for(int y = 0; y < src.get_size().height; ++y)
  {
    memcpy(dst.get_row_data(src.get_size().height - y - 1),
           src.get_row_data(y),
           static_cast<size_t>(src.get_pitch()));
  }

  return SoftwareSurface(std::move(dst));
}

SoftwareSurface
SoftwareSurface::crop(Rect const& rect_in) const
{
  // FIXME: We could do a crop without copying content, sm_imply
  // reference the old SoftwareSurfaceM_Impl and have a different pitch
  // and pixel offset
  assert(rect_in.is_normal());

  // Clip the rectangle to the image
  Rect rect(Math::clamp(0, rect_in.left,   get_width()),
            Math::clamp(0, rect_in.top,    get_height()),
            Math::clamp(0, rect_in.right,  get_width()),
            Math::clamp(0, rect_in.bottom, get_height()));

  PixelData const& src = *m_pixel_data;
  PixelData dst(src.get_format(), rect.get_size());

  for(int y = rect.top; y < rect.bottom; ++y)
  {
    memcpy(dst.get_row_data(y - rect.top),
           src.get_row_data(y) + rect.left * get_bytes_per_pixel(),
           static_cast<size_t>(rect.get_width() * get_bytes_per_pixel()));
  }

  return SoftwareSurface(std::move(dst));
}

Size
SoftwareSurface::get_size()  const
{
  return m_pixel_data->get_size();
}

int
SoftwareSurface::get_width()  const
{
  return m_pixel_data->get_width();
}

int
SoftwareSurface::get_height() const
{
  return m_pixel_data->get_height();
}

PixelData::Format
SoftwareSurface::get_format() const
{
  return m_pixel_data->get_format();
}

RGB
SoftwareSurface::get_average_color() const
{
  PixelData const& src = *m_pixel_data;

  assert(src.get_format() == PixelData::RGB_FORMAT);
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

SoftwareSurface
SoftwareSurface::to_rgb() const
{
  PixelData const& src = *m_pixel_data;

  switch(src.get_format())
  {
    case PixelData::RGB_FORMAT:
      return *this;

    case PixelData::RGBA_FORMAT:
    {
      PixelData pixel_data(PixelData::RGB_FORMAT, src.get_size());

      int num_pixels = get_width() * get_height();
      uint8_t const* src_pixels = src.get_data();
      uint8_t* dst_pixels = pixel_data.get_data();

      for(int i = 0; i < num_pixels; ++i)
      {
        dst_pixels[3*i+0] = src_pixels[4*i+0];
        dst_pixels[3*i+1] = src_pixels[4*i+1];
        dst_pixels[3*i+2] = src_pixels[4*i+2];
      }

      return SoftwareSurface(std::move(pixel_data));
    }

    default:
      assert(false && "SoftwareSurface::to_rgb: Unknown format");
      return {};
  }
}

void
SoftwareSurface::get_pixel(int x, int y, RGB& rgb) const
{
  m_pixel_data->get_pixel(x, y, rgb);
}

void
SoftwareSurface::get_pixel(int x, int y, RGBA& rgb) const
{
  m_pixel_data->get_pixel(x, y, rgb);
}

int
SoftwareSurface::get_bytes_per_pixel() const
{
  return m_pixel_data->get_bytes_per_pixel();
}

/* EOF */
