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

#ifndef HEADER_SURF_PLUGINS_EXIF_HPP
#define HEADER_SURF_PLUGINS_EXIF_HPP

#ifdef HAVE_EXIF

#include <filesystem>
#include <string>
#include <span>

#include <surf/pixel_data.hpp>
#include <surf/transform.hpp>

namespace surf {
namespace exif {

Transform get_orientation(std::filesystem::path const& filename);
Transform get_orientation(std::span<uint8_t const> data);

} // namespace exif
} // namespace surf

#endif

#endif

/* EOF */
