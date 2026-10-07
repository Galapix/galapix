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

#include "transform.hpp"

#include <format>
#include <stdexcept>

namespace surf {

Transform transform_from_string(std::string_view text)
{
  if (text == "0") {
    return surf::Transform::ROTATE_0;
  } else if (text == "90") {
    return surf::Transform::ROTATE_90;
  } else if (text == "180") {
    return surf::Transform::ROTATE_180;
  } else if (text == "270") {
    return surf::Transform::ROTATE_270;
  } else if (text == "0flip") {
    return surf::Transform::ROTATE_0_FLIP;
  } else if (text == "90flip") {
    return surf::Transform::ROTATE_90_FLIP;
  } else if (text == "180flip") {
    return surf::Transform::ROTATE_180_FLIP;
  } else if (text == "270flip") {
    return surf::Transform::ROTATE_270_FLIP;
  } else if (text == "flip") {
    return surf::Transform::ROTATE_0_FLIP;
  } else if (text == "vflip") {
    return surf::Transform::FLIP_VERTICAL;
  } else if (text == "hflip") {
    return surf::Transform::FLIP_HORIZONTAL;
  } else {
    throw std::invalid_argument(std::format("not a valid transform: {}", text));
  }
}

} // namespace surf

/* EOF */
