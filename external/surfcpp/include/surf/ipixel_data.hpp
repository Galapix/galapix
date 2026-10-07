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

#ifndef HEADER_SURF_IPIXEL_DATA_HPP
#define HEADER_SURF_IPIXEL_DATA_HPP

#include <memory>

#include <geom/fwd.hpp>

#include "fwd.hpp"

namespace surf {

class IPixelData
{
public:
  virtual ~IPixelData() {}
  virtual PixelFormat get_format() const = 0;
  virtual geom::isize get_size() const = 0;
  virtual int get_width() const = 0;
  virtual int get_height() const = 0;
  virtual int get_row_length() const = 0;
  virtual int get_pitch() const = 0;
  virtual void* get_row_data(int y) = 0;
  virtual void const* get_row_data(int y) const = 0;
  virtual void put_pixel_color(geom::ipoint const& pos, Color const& color) = 0;
  virtual Color get_pixel_color(geom::ipoint const& pos) const = 0;
  virtual bool empty() const = 0;
  virtual std::unique_ptr<IPixelData> copy() const = 0;
  virtual std::unique_ptr<IPixelData> create_view(geom::irect const& rect) = 0;
  virtual std::unique_ptr<IPixelData const> create_view(geom::irect const& rect) const = 0;

  bool operator==(IPixelData const& rhs) const {
    return is_equal(rhs);
  }

  bool operator!=(IPixelData const& rhs) const {
    return !is_equal(rhs);
  }

protected:
  virtual bool is_equal(IPixelData const&) const = 0;
};

} // namespace surf

#endif

/* EOF */
