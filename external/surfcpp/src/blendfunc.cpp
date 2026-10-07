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

#include "blendfunc.hpp"

namespace surf {

BlendFunc BlendFunc_from_string(std::string_view blendfunc_str)
{
  if (blendfunc_str == "COPY") {
    return BlendFunc::COPY;
  } else if (blendfunc_str == "BLEND") {
    return BlendFunc::BLEND;
  } else if (blendfunc_str == "ADD") {
    return BlendFunc::ADD;
  } else if (blendfunc_str == "MULTIPLY") {
    return BlendFunc::MULTIPLY;
  } else {
    throw std::invalid_argument("invalid BlendFunc");
  }
}

} // namespace surf

/* EOF */
