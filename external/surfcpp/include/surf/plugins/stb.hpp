// surf - Software surface library
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SURF_PLUGINS_STB_HPP
#define HEADER_SURF_PLUGINS_STB_HPP

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include <surf/fwd.hpp>

namespace surf {
namespace stb {

/** PNG and JPEG via the header-only stb_image and stb_image_write,
    used instead of libpng and libjpeg when surf is built WITH_STB */

SoftwareSurface load_from_file(std::filesystem::path const& filename);
SoftwareSurface load_from_mem(std::span<uint8_t const> data);

void save_png(SoftwareSurface const& surface, std::filesystem::path const& filename);
std::vector<uint8_t> save_png(SoftwareSurface const& surface);

void save_jpeg(SoftwareSurface const& surface, std::filesystem::path const& filename, int quality);
std::vector<uint8_t> save_jpeg(SoftwareSurface const& surface, int quality);

void register_loader(SoftwareSurfaceFactory& factory);

} // namespace stb
} // namespace surf

#endif

/* EOF */
