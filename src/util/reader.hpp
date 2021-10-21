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

#ifndef HEADER_GALAPIX_UTIL_READER_HPP
#define HEADER_GALAPIX_UTIL_READER_HPP

#include <vector>
#include <memory>

#include <prio/reader_collection.hpp>
#include <prio/reader_document.hpp>
#include <prio/reader_mapping.hpp>
#include <prio/reader_object.hpp>
#include <geom/fwd.hpp>
#include <surf/fwd.hpp>

#include "math/vector2i.hpp"
#include "math/vector2f.hpp"
#include "math/rect.hpp"

namespace surf {
class RGBA;
} // namespace surf

class URL;

using ReaderCollection = prio::ReaderCollection;
using ReaderDocument = prio::ReaderDocument;
using ReaderMapping = prio::ReaderMapping;
using ReaderObject = prio::ReaderObject;

namespace prio {

template<> bool read_custom(ReaderMapping const& map, std::string_view key, URL& value);
template<> bool read_custom(ReaderMapping const& map, std::string_view key, Vector2i& value);
template<> bool read_custom(ReaderMapping const& map, std::string_view key, Vector2f& value);
template<> bool read_custom(ReaderMapping const& map, std::string_view key, Rect& value);
template<> bool read_custom(ReaderMapping const& map, std::string_view key, geom::isize& value);
template<> bool read_custom(ReaderMapping const& map, std::string_view key, surf::Color& value);

} // namespace prio

#endif

/* EOF */
