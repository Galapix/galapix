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

#include "plugins/ufraw.hpp"

#include <stdexcept>

#include "plugins/pnm.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/url.hpp"

bool
UFRaw::is_available()
{
  try 
  {
    std::string exe = Filesystem::find_exe("ufraw-batch", "GALAPIX_UFRAW_BATCH");
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
UFRaw::load_from_file(const std::string& filename)
{
  Exec ufraw(Filesystem::find_exe("ufraw-batch", "GALAPIX_UFRAW_BATCH"));
  ufraw
    .arg(filename)
    .arg("--silent")
    .arg("--out-type=ppm8")
    .arg("--output=-");
     
  if (ufraw.exec())
  {
    throw std::runtime_error("UFRaw::load_from_file(): " + std::string(ufraw.get_stderr().begin(), ufraw.get_stderr().end()));
  }
  else
  {
    return PNM::load_from_mem(&*ufraw.get_stdout().begin(), ufraw.get_stdout().size());
  }
}

/* EOF */
