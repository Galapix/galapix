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

#include "surface/software_surface_float.hpp"

#include <geom/size.hpp>

#include "math/math.hpp"

SoftwareSurfaceFloatPtr
SoftwareSurfaceFloat::create(const Size& size)
{
  return SoftwareSurfaceFloatPtr(new SoftwareSurfaceFloat(size));
}

SoftwareSurfaceFloat::SoftwareSurfaceFloat(const Size& size) :
  m_size(size),
  m_pixels(static_cast<size_t>(geom::area(size) * 4))
{
}

SoftwareSurfaceFloatPtr
SoftwareSurfaceFloat::from_software_surface(SoftwareSurface const& surface)
{
  PixelData const& src = surface.get_pixel_data();
  SoftwareSurfaceFloatPtr surfacef = SoftwareSurfaceFloat::create(surface.get_size());
  for(int y = 0; y < surface.get_height(); ++y)
  {
    for(int x = 0; x < surface.get_width(); ++x)
    {
      RGBA rgba;
      src.get_pixel(x, y, rgba);

      RGBAf rgbaf = RGBAf::from_rgba(rgba);
      surfacef->put_pixel(x, y, rgbaf);
    }
  }
  return surfacef;
}

void
SoftwareSurfaceFloat::apply_gamma(float gamma)
{
  for(int y = 0; y < m_size.height(); ++y)
  {
    for(int x = 0; x < m_size.width(); ++x)
    {
      RGBAf rgba;
      get_pixel(x, y, rgba);
      rgba.r = powf(rgba.r, gamma);
      rgba.g = powf(rgba.g, gamma);
      rgba.b = powf(rgba.b, gamma);
      put_pixel(x, y, rgba);
    }
  }
}

Size
SoftwareSurfaceFloat::get_size() const
{
  return m_size;
}

int
SoftwareSurfaceFloat::get_width() const
{
  return m_size.width();
}

int
SoftwareSurfaceFloat::get_height() const
{
  return m_size.height();
}

int
SoftwareSurfaceFloat::get_pitch() const
{
  return m_size.width() * 4;
}

void
SoftwareSurfaceFloat::put_pixel(int x, int y, const RGBAf& rgba)
{
  m_pixels[get_pitch() * y + 4*x + 0] = rgba.r;
  m_pixels[get_pitch() * y + 4*x + 1] = rgba.g;
  m_pixels[get_pitch() * y + 4*x + 2] = rgba.b;
  m_pixels[get_pitch() * y + 4*x + 3] = rgba.a;
}

void
SoftwareSurfaceFloat::get_pixel(int x, int y, RGBAf& rgba) const
{
  rgba.r = m_pixels[get_pitch() * y + 4*x + 0];
  rgba.g = m_pixels[get_pitch() * y + 4*x + 1];
  rgba.b = m_pixels[get_pitch() * y + 4*x + 2];
  rgba.a = m_pixels[get_pitch() * y + 4*x + 3];
}

SoftwareSurface
SoftwareSurfaceFloat::to_software_surface() const
{
  PixelData dst(PixelData::RGBA_FORMAT, m_size);
  for(int y = 0; y < m_size.height(); ++y)
  {
    for(int x = 0; x < m_size.width(); ++x)
    {
      RGBAf rgbaf;
      get_pixel(x, y, rgbaf);

      RGBA rgba;
      rgba.r = static_cast<uint8_t>(Math::clamp(0, static_cast<int>(rgbaf.r * 255.0f), 255));
      rgba.g = static_cast<uint8_t>(Math::clamp(0, static_cast<int>(rgbaf.g * 255.0f), 255));
      rgba.b = static_cast<uint8_t>(Math::clamp(0, static_cast<int>(rgbaf.b * 255.0f), 255));
      rgba.a = static_cast<uint8_t>(Math::clamp(0, static_cast<int>(rgbaf.a * 255.0f), 255));
      dst.put_pixel(x, y, rgba);
    }
  }
  return SoftwareSurface(std::move(dst));
}

/* EOF */
