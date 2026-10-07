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

#ifndef HEADER_SURF_PIXEL_DATA_HPP
#define HEADER_SURF_PIXEL_DATA_HPP

#include <stdexcept>
#include <vector>

#include "pixel_view.hpp"

namespace surf {

template<typename Pixel>
class PixelData : public PixelView<Pixel>
{
public:
  PixelData() :
    PixelView<Pixel>(),
    m_pixels_ownership()
  {}

  PixelData(PixelData<Pixel> const& other) :
    PixelView<Pixel>(other),
    m_pixels_ownership(other.m_pixels_ownership)
  {
    this->m_pixels = m_pixels_ownership.data();
  }

  PixelData<Pixel>& operator=(PixelData<Pixel> const& other)
  {
    if (this != &other) {
      PixelView<Pixel>::operator=(other);
      m_pixels_ownership = other.m_pixels_ownership;
      this->m_pixels = m_pixels_ownership.data();
    }
    return *this;
  }

  PixelData(PixelData<Pixel>&& other) noexcept :
    PixelView<Pixel>(other),
    m_pixels_ownership(std::move(other.m_pixels_ownership))
  {
    this->m_pixels = m_pixels_ownership.data();
    other.reset();
  }

  PixelData<Pixel>& operator=(PixelData<Pixel>&& other) noexcept
  {
    if (this != &other) {
      PixelView<Pixel>::operator=(other);
      m_pixels_ownership = std::move(other.m_pixels_ownership);
      this->m_pixels = m_pixels_ownership.data();
      other.reset();
    }
    return *this;
  }

  PixelData(PixelView<Pixel> const& view) :
    PixelView<Pixel>(view.get_size(), static_cast<Pixel*>(nullptr)),
    m_pixels_ownership(geom::area(this->m_size))
  {
    // the copy is tightly packed, independent of the view's pitch
    this->m_pixels = m_pixels_ownership.data();
    for (int y = 0; y < this->m_size.height(); ++y) {
      std::copy_n(view.get_row(y), this->m_size.width(), this->get_row(y));
    }
  }

  PixelData(geom::isize const& size, Pixel const& pixel = {}) :
    PixelView<Pixel>(size, nullptr),
    m_pixels_ownership(geom::area(size), pixel)
  {
    this->m_pixels = m_pixels_ownership.data();
  }

  PixelData(geom::isize const& size, std::vector<Pixel> pixels) :
    PixelData(size, std::move(pixels), size.width())
  {}

  PixelData(geom::isize const& size, std::vector<Pixel> pixels, int row_length) :
    PixelView<Pixel>(size, static_cast<Pixel*>(nullptr), row_length),
    m_pixels_ownership(std::move(pixels))
  {
    if (!size.is_valid() || row_length < size.width() ||
        (size.height() > 0 &&
         m_pixels_ownership.size() < static_cast<size_t>(row_length) * static_cast<size_t>(size.height() - 1) + static_cast<size_t>(size.width()))) {
      throw std::invalid_argument("PixelData: pixel vector too small for the given size");
    }
    this->m_pixels = m_pixels_ownership.data();
  }

private:
  /** Leave a moved-from object as a valid empty PixelData */
  void reset() {
    this->m_size = geom::isize(0, 0);
    this->m_pitch = 0;
    this->m_pixels = nullptr;
    m_pixels_ownership.clear();
  }

private:
  std::vector<Pixel> m_pixels_ownership;
};

} // namespace surf

#endif

/* EOF */
