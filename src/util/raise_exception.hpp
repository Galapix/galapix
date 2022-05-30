/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_UTIL_RAISE_EXCEPTION_HPP
#define HEADER_GALAPIX_UTIL_RAISE_EXCEPTION_HPP

#include <sstream>
#include <stdexcept>

#define raise_exception(type, expr) do {  \
  std::ostringstream b42465a70169; \
  b42465a70169 << __FILE__ << ":" << __LINE__ << ":" << expr; /* NOLINT */ \
  throw type(b42465a70169.str()); \
} while(false)

#define raise_runtime_error(expr) raise_exception(std::runtime_error, expr)

namespace galapix {

} // namespace galapix

#endif

/* EOF */
