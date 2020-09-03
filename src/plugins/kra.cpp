// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "plugins/kra.hpp"

#include <logmich/log.hpp>

#include "plugins/png.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"

bool
KRA::is_available()
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

SoftwareSurface
KRA::load_from_file(const std::string& filename)
{
  // koconverter --batch --mimetype image/png sketch39.kra /dev/stdout  > /tmp/test.png
  Exec koconverter("koconverter");
  koconverter.arg("--batch").arg("--mimetype").arg("image/png");
  koconverter.arg(filename).arg("/dev/stdout");
  koconverter.exec();

  return PNG::load_from_mem({reinterpret_cast<uint8_t const*>(koconverter.get_stdout().data()), koconverter.get_stdout().size()});
}

// SoftwareSurface
// KRA::load_from_mem(std::span<uint8_t const> data)
// {
//   Exec koconverter("koconverter");
//   koconverter.arg("--batch").arg("--mimetype").arg("image/png");
//   koconverter.arg("/dev/stdin").arg("/dev/stdout");
//   koconverter.set_stdin(Blob(data, len));
//   koconverter.exec();

//   return PNG::load_from_mem((uint8_t*)&*koconverter.get_stdout().begin(), koconverter.get_stdout().size());
// }

/* EOF */
