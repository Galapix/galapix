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

#include <stdexcept>

#include "pixel_data.hpp"
#ifdef HAVE_STB
#  include "plugins/stb.hpp"
#else
#  include "plugins/jpeg.hpp"
#  include "plugins/png.hpp"
#endif
#include "save.hpp"
#include "util/filesystem.hpp"

namespace surf {

namespace {

void save_png(SoftwareSurface const& surface, std::filesystem::path const& path)
{
#ifdef HAVE_STB
  stb::save_png(surface, path);
#else
  png::save(surface, path);
#endif
}

void save_jpeg(SoftwareSurface const& surface, std::filesystem::path const& path, int quality)
{
#ifdef HAVE_STB
  stb::save_jpeg(surface, path, quality);
#else
  jpeg::save(surface, path, quality);
#endif
}

} // namespace

void save(SoftwareSurface const& surface, std::filesystem::path const& path, std::string_view format)
{
  if (format == "auto") {
    std::string const extension = Filesystem::get_extension(path);
    if (extension == "jpg" || extension == "jpeg") {
      save_jpeg(surface, path, 70);
    } else if (extension == "png") {
      save_png(surface, path);
    } else {
      throw std::invalid_argument("unknown file extension: " + path.string());
    }
  } else if (format == "png") {
    save_png(surface, path);
  } else if (format == "jpeg") {
    save_jpeg(surface, path, 70);
  } else {
    throw std::runtime_error("unsupported format");
  }
}

} // namespace surf

/* EOF */
