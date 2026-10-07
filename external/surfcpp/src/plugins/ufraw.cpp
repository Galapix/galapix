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

#include "plugins/ufraw.hpp"

#include <stdexcept>

#include <logmich/log.hpp>

#include "plugins/pnm.hpp"
#include "software_surface_factory.hpp"
#include "software_surface_loader.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"

namespace surf {
namespace ufraw {

bool is_available()
{
  try
  {
    std::string exe = Filesystem::find_exe("ufraw-batch");
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
  Exec ufraw("ufraw-batch");
  ufraw
    .arg(filename.string())
    .arg("--silent")
    .arg("--out-type=ppm8")
    .arg("--output=-");

  if (ufraw.exec())
  {
    throw std::runtime_error("UFRaw::load_from_file(): " + std::string(ufraw.get_stderr().begin(), ufraw.get_stderr().end()));
  }
  else
  {
    return pnm::load_from_mem(ufraw.get_stdout());
  }
}

void register_loader(SoftwareSurfaceFactory& factory)
{
  auto loader = make_loader("ufraw", load_from_file, nullptr);

  factory.register_by_extension(*loader, "3fr");
  factory.register_by_extension(*loader, "arw");
  factory.register_by_extension(*loader, "cr2");
  factory.register_by_extension(*loader, "crw");
  factory.register_by_extension(*loader, "dcr");
  factory.register_by_extension(*loader, "dcs");
  factory.register_by_extension(*loader, "dng");
  factory.register_by_extension(*loader, "kdc");
  factory.register_by_extension(*loader, "mef");
  factory.register_by_extension(*loader, "mrw");
  factory.register_by_extension(*loader, "nef");
  factory.register_by_extension(*loader, "orf");
  factory.register_by_extension(*loader, "pef");
  factory.register_by_extension(*loader, "raf");
  factory.register_by_extension(*loader, "raw");
  factory.register_by_extension(*loader, "sr2");
  factory.register_by_extension(*loader, "srf");
  factory.register_by_extension(*loader, "x3f");

  factory.register_by_mime_type(*loader, "image/x-adobe-dng");
  factory.register_by_mime_type(*loader, "image/x-canon-cr2 ");
  factory.register_by_mime_type(*loader, "image/x-canon-crw");
  factory.register_by_mime_type(*loader, "image/x-fuji-raf ");
  factory.register_by_mime_type(*loader, "image/x-minolta-mrw");
  factory.register_by_mime_type(*loader, "image/x-nikon-nef");
  factory.register_by_mime_type(*loader, "image/x-olympus-orf");
  factory.register_by_mime_type(*loader, "image/x-panasonic-raw ");
  factory.register_by_mime_type(*loader, "image/x-panasonic-raw");
  factory.register_by_mime_type(*loader, "image/x-pentax-pef");
  factory.register_by_mime_type(*loader, "image/x-raw");
  factory.register_by_mime_type(*loader, "image/x-sony-arw");
  factory.register_by_mime_type(*loader, "image/x-sony-sr2");
  factory.register_by_mime_type(*loader, "image/x-sony-srf");

  factory.add_loader(std::move(loader));
}

} // namespace ufraw
} // namespace surf

/* EOF */
