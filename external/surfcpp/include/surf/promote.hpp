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

#ifndef HEADER_SURF_PROMOTE_HPP
#define HEADER_SURF_PROMOTE_HPP

#include <cstdint>
#include <type_traits>

namespace surf {

template<typename A, typename B>
struct promote_t
{
  using type = typename std::conditional<(sizeof(A) < 4 && sizeof(B) < 4),
                                         uint32_t,
                                         uint64_t
                                         >::type;
};

template<typename A, typename B>
typename promote_t<A, B>::type promote(A a) {
  return static_cast<typename promote_t<A, B>::type>(a);
}

/** Type wide enough to sum or multiply a few values of T without
    overflow, T itself for floating point */
template<typename T>
using accumulate_t = typename std::conditional<std::is_floating_point<T>::value,
                                               T,
                                               typename promote_t<T, T>::type>::type;

} // namespace surf

#endif

/* EOF */
