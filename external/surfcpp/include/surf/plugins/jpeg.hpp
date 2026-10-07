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

#ifndef HEADER_SURF_PLUGINS_JPEG_HPP
#define HEADER_SURF_PLUGINS_JPEG_HPP

#include <filesystem>
#include <functional>
#include <span>
#include <string>

#include <jpeglib.h>

#include <surf/fwd.hpp>
#include <surf/pixel_data.hpp>
#include <surf/software_surface_factory.hpp>

namespace surf {
namespace jpeg {

bool filename_is_jpeg(std::filesystem::path const& filename);

geom::isize get_size(std::filesystem::path const& filename);
geom::isize get_size(std::span<uint8_t const> data);

/** Load a PixelData from the filesystem

    @param[in]  filename Filename of the file to load
    @param[in]  scale    Scale the image by 1/scale (only 1,2,4,8 allowed)
    @param[out] size     The size of the unscaled image

    @return reference counted pointer to a PixelData object */
SoftwareSurface load_from_file(std::filesystem::path const& filename, int scale = 1, geom::isize* size = nullptr);

/** Load a JPEG from memory

    @param[in]  data  Address of the JPEG data
    @param[in]  len   Length of the JPEG data
    @param[out] size  The size of the unscaled image

    @return reference counted pointer to a PixelData object */
SoftwareSurface load_from_mem(std::span<uint8_t const> data, int scale = 1, geom::isize* size = nullptr);

void save(SoftwareSurface const& surface, std::filesystem::path const& filename, int quality);
std::vector<uint8_t> save(SoftwareSurface const& surface, int quality);

void register_loader(SoftwareSurfaceFactory& factory);

} // namespace jpeg
} // namespace surf

#endif

/* EOF */
