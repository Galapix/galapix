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

#ifndef HEADER_SURF_PLUGINS_JPEG_MEMORY_SRC_HPP
#define HEADER_SURF_PLUGINS_JPEG_MEMORY_SRC_HPP

#include <span>
#include <stdint.h>
#include <stdio.h>
#include <jpeglib.h>

namespace surf {

/** Setup IO handling so that a JPEG can be read from memory */
void jpeg_memory_src(j_decompress_ptr cinfo, std::span<uint8_t const> data);

} // namespace surf

#endif

/* EOF */
