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

#ifndef HEADER_SURF_PLUGINS_KRA_HPP
#define HEADER_SURF_PLUGINS_KRA_HPP

#include <filesystem>
#include <string>

#include <surf/fwd.hpp>
#include <surf/pixel_data.hpp>

namespace surf {
namespace kra {

bool is_available();
SoftwareSurface load_from_file(std::filesystem::path const& filename);
void register_loader(SoftwareSurfaceFactory& factory);

} // namespace kra
} // namespace surf

#endif

/* EOF */
