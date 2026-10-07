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

#include "plugins/rsvg.hpp"

#include <stdexcept>
#include <logmich/log.hpp>

#include "plugins/png.hpp"
#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"

namespace surf {
namespace rsvg {

bool is_available()
{
  try
  {
    std::string exe = Filesystem::find_exe("rsvg");
    log_info("found {}", exe);
    return true;
  }
  catch(std::exception& err)
  {
    log_warn(err.what());
    return false;
  }
}

SoftwareSurface load_from_file(std::filesystem::path const& filename)
{
  Exec rsvg("rsvg");

  rsvg.arg("--format").arg("png");
  rsvg.arg(filename.string());
  rsvg.arg("/dev/stdout");

  if (rsvg.exec() != 0) {
    throw std::runtime_error("RSVG::load_from_file(): " + std::string(rsvg.get_stderr().begin(), rsvg.get_stderr().end()));
  }

  return png::load_from_mem(rsvg.get_stdout());
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("rsvg", load_from_file, nullptr);

  factory.register_by_extension(*loader, "svg");
  factory.register_by_extension(*loader, "svgz");

  factory.register_by_mime_type(*loader, "image/svg+xml");

  factory.add_loader(std::move(loader));
}

} // namespace rsvg
} // namespace surf

/* EOF */
