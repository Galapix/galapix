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

#include "plugins/vidthumb.hpp"

#include <stdexcept>
#include <sstream>

#include <logmich/log.hpp>

#include "plugins/png.hpp"
#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"

namespace surf {
namespace vidthumb {

bool is_available()
{
  try
  {
    std::string exe = Filesystem::find_exe("vidthumb");
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
  Exec vidthumb("vidthumb");

  std::ostringstream out;
  out << "/tmp/vidthumb.tmp." << rand() << ".png";

  vidthumb
    .arg("-o").arg(out.str())
    .arg(filename.string());

  if (vidthumb.exec() == 0)
  {
    SoftwareSurface pixeldata = png::load_from_file(out.str());
    remove(out.str().c_str());
    return pixeldata;
  }
  else
  {
    throw std::runtime_error("VidThumb::load_from_file(): " + std::string(vidthumb.get_stderr().begin(),
                                                                          vidthumb.get_stderr().end()));
  }
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("vidthumb", load_from_file, nullptr);

  factory.register_by_extension(*loader, "avi");
  factory.register_by_extension(*loader, "wmv");
  factory.register_by_extension(*loader, "flv");
  factory.register_by_extension(*loader, "ogv");
  factory.register_by_extension(*loader, "mkv");
  factory.register_by_extension(*loader, "mp4");

  factory.add_loader(std::move(loader));
}

} // namespace vidthumb
} // namespace surf

/* EOF */
