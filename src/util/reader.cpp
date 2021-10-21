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

#include "util/reader.hpp"

#include <surf/color.hpp>

#include "util/url.hpp"

namespace prio {

template<>
bool read_custom(ReaderMapping const& map, std::string_view key, URL& value_out)
{
  std::string filename;
  if (!map.read(key, filename)) {
    return false;
  }

  value_out = URL::from_string(filename);
  return true;
}

template<>
bool read_custom(ReaderMapping const& map, std::string_view key, Vector2i& value_out)
{
  std::vector<int> v;
  if (!map.read(key, v)) {
    return false;
  }

  if (v.size() != 2) {
    return false;
  }

  value_out = Vector2i(v[0], v[1]);
  return true;
}

template<>
bool read_custom(ReaderMapping const& map, std::string_view key, Vector2f& value_out)
{
  std::vector<float> v;
  if (!map.read(key, v)) {
    return false;
  }

  if (v.size() != 2) {
    return false;
  }

  value_out = Vector2f(v[0], v[1]);
  return true;

}

template<>
bool read_custom(ReaderMapping const& map, std::string_view key, geom::irect& value_out)
{
  std::vector<int> v;
  if (!map.read(key, v)) {
    return false;
  }

  if (v.size() != 4) {
    return false;
  }

  value_out = geom::irect(v[0], v[1], v[2], v[3]);
  return true;
}

template<>
bool read_custom(ReaderMapping const& map, std::string_view key, geom::isize& value_out)
{
  std::vector<int> v;
  if (!map.read(key, v)) {
    return false;
  }

  if (v.size() != 2) {
    return false;
  }

  value_out = geom::isize(v[0], v[1]);
  return true;
}

template<>
bool read_custom(ReaderMapping const& map, std::string_view key, surf::Color& value_out)
{
  std::vector<int> v;
  if (!map.read(key, v)) {
    return false;
  }

  if (v.size() != 4) {
    return false;
  }

  value_out.r = static_cast<uint8_t>(v[0]);
  value_out.g = static_cast<uint8_t>(v[1]);
  value_out.b = static_cast<uint8_t>(v[2]);
  value_out.a = static_cast<uint8_t>(v[3]);
  return true;
}

} // namespace prio

/* EOF */
