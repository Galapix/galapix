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

#ifndef HEADER_SURF_SOFTWARE_SURFACE_HPP
#define HEADER_SURF_SOFTWARE_SURFACE_HPP

#include <stdint.h>

#include <filesystem>
#include <memory>

#include <geom/fwd.hpp>

#include "fwd.hpp"
#include "blendfunc.hpp"
#include "pixel_data.hpp"

namespace surf {

class SoftwareSurface
{
public:
  static SoftwareSurface from_file(std::filesystem::path const& filename);
  static SoftwareSurface from_file(std::filesystem::path const& filename, std::string_view loader);
  static SoftwareSurface create(PixelFormat format, geom::isize const& size, Color const& color = {});

  template<typename Pixel>
  static SoftwareSurface create_view(PixelView<Pixel>& data) {
    return SoftwareSurface(std::make_unique<PixelView<Pixel>>(data));
  }

  template<typename Pixel>
  static SoftwareSurface create_view(PixelView<Pixel> const& data) {
    return SoftwareSurface(std::make_unique<PixelView<Pixel>>(data));
  }

  static SoftwareSurface create_view(PixelFormat format, geom::isize const& size, void* ptr, int pitch);
  static SoftwareSurface create_view(PixelFormat format, geom::isize const& size, void const* ptr, int pitch);

public:
  SoftwareSurface();
  SoftwareSurface(SoftwareSurface const& other);
  SoftwareSurface(SoftwareSurface&& other) = default;

  SoftwareSurface(std::unique_ptr<IPixelData> pixel_data) :
    m_pixel_data(std::move(pixel_data))
  {}

  template<typename Pixel>
  explicit SoftwareSurface(PixelData<Pixel> data) :
    m_pixel_data(std::make_unique<PixelData<Pixel>>(std::move(data)))
  {}

  template<typename Pixel>
  explicit SoftwareSurface(PixelView<Pixel> const& data) :
    m_pixel_data(std::make_unique<PixelData<Pixel>>(data))
  {}

  SoftwareSurface& operator=(SoftwareSurface const& other);
  SoftwareSurface& operator=(SoftwareSurface&& other) = default;

  geom::isize get_size() const;
  int get_width() const;
  int get_height() const;
  int get_pitch() const;
  PixelFormat get_format() const;

  Color get_pixel(geom::ipoint const& position) const;
  void put_pixel(geom::ipoint const& position, Color const& color);

  void* get_data();
  void* get_row_data(int y);

  void const* get_data() const;
  void const* get_row_data(int y) const;

  IPixelData const& get_pixel_data() const { return pixel_data(); }

  template<typename Pixel>
  PixelView<Pixel> const* as_pixelview_ptr() const {
    return dynamic_cast<PixelView<Pixel> const*>(m_pixel_data.get());
  }

  template<typename Pixel>
  PixelView<Pixel> const& as_pixelview() const {
    return dynamic_cast<PixelView<Pixel> const&>(pixel_data());
  }

  template<typename Pixel>
  PixelView<Pixel>& as_pixelview() {
    return dynamic_cast<PixelView<Pixel>&>(pixel_data());
  }

  bool operator==(SoftwareSurface const& rhs) const {
    if (!m_pixel_data || !rhs.m_pixel_data) {
      return !m_pixel_data && !rhs.m_pixel_data;
    }
    return *m_pixel_data == *rhs.m_pixel_data;
  }

  SoftwareSurface get_view(geom::irect const& rect) const;

private:
  /** Throws if the surface is empty */
  IPixelData& pixel_data();
  IPixelData const& pixel_data() const;

private:
  std::unique_ptr<IPixelData> m_pixel_data;
};

void blit(SoftwareSurface const& src, SoftwareSurface& dst, geom::ipoint const& pos);
void blit(SoftwareSurface const& src, geom::irect const& srcrect, SoftwareSurface& dst, geom::ipoint const& pos);

void blend_scaled(BlendFunc blendfunc, SoftwareSurface const& src, SoftwareSurface& dst, geom::irect const& dstrect);
void blend_scaled(BlendFunc blendfunc, SoftwareSurface const& src, geom::irect const& srcrect, SoftwareSurface& dst, geom::irect const& dstrect);

void blend(BlendFunc blendfunc, SoftwareSurface const& src, SoftwareSurface& dst, geom::ipoint const& pos);
void blend(BlendFunc blendfunc, SoftwareSurface const& src, geom::irect const& srcrect, SoftwareSurface& dst, geom::ipoint const& pos);

void fill(SoftwareSurface& dst, Color const& color);
void fill_rect(SoftwareSurface& dst, geom::irect const& rect, Color const& color);

SoftwareSurface convert(SoftwareSurface const& src, PixelFormat format);

} // namespace surf

#endif

/* EOF */
