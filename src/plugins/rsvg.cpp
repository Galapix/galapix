/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "plugins/rsvg.hpp"

#include <stdexcept>

#include "plugins/png.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/url.hpp"

bool
RSVG::is_available()
{
  try 
  {
    std::string exe = Filesystem::find_exe("rsvg", "GALAPIX_RSVG");
    log_info << "found " << exe << std::endl;
    return true;
  }
  catch(std::exception& err)
  {
    log_warning << err.what() << std::endl;
    return false;
  }
}

SoftwareSurfacePtr
RSVG::load_from_file(const std::string& filename)
{
  Exec rsvg(Filesystem::find_exe("rsvg", "GALAPIX_RSVG"));

  rsvg.arg("--format").arg("png");
  rsvg.arg(filename);
  rsvg.arg("/dev/stdout");

  if (rsvg.exec() == 0)
  {
    BlobPtr blob = Blob::copy(&*rsvg.get_stdout().begin(), rsvg.get_stdout().size());
    SoftwareSurfacePtr surface = PNG::load_from_mem(blob->get_data(), blob->size());
    return surface;
  }
  else
  {
    throw std::runtime_error("RSVG::load_from_file(): " + std::string(rsvg.get_stderr().begin(), rsvg.get_stderr().end()));
  }
}

/* EOF */
