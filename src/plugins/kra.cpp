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

#include "plugins/kra.hpp"

#include <logmich/log.hpp>

#include "plugins/png.hpp"
#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"

namespace surf {
namespace kra {

bool is_available()
{
  try
  {
    std::string exe = Filesystem::find_exe("koconverter");
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
  // koconverter --batch --mimetype image/png sketch39.kra /dev/stdout  > /tmp/test.png
  Exec koconverter("koconverter");
  koconverter.arg("--batch").arg("--mimetype").arg("image/png");
  koconverter.arg(filename.string()).arg("/dev/stdout");
  koconverter.exec();

  return png::load_from_mem(koconverter.get_stdout());
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("kra", load_from_file, nullptr);

  factory.register_by_extension(*loader, "kra");

  factory.register_by_mime_type(*loader, "application/x-krita");

  factory.add_loader(std::move(loader));
}

// PixelData
// KRA::load_from_mem(std::span<uint8_t const> data)
// {
//   Exec koconverter("koconverter");
//   koconverter.arg("--batch").arg("--mimetype").arg("image/png");
//   koconverter.arg("/dev/stdin").arg("/dev/stdout");
//   koconverter.set_stdin(Blob(data, len));
//   koconverter.exec();

//   return png::load_from_mem((uint8_t*)&*koconverter.get_stdout().begin(), koconverter.get_stdout().size());
// }

} // namespace kra
} // namespace surf

/* EOF */
