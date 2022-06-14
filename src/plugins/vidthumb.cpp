/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#include "plugins/vidthumb.hpp"

#include <stdexcept>
#include <sstream>

#include "plugins/png.hpp"
#include "util/exec.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"

bool
VidThumb::is_available()
{
  try 
  {
    std::string exe = Filesystem::find_exe("vidthumb", "GALAPIX_VIDTHUMB");
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
VidThumb::load_from_file(const std::string& filename)
{
  Exec vidthumb(Filesystem::find_exe("vidthumb", "GALAPIX_VIDTHUMB"));

  std::ostringstream out;
  out << "/tmp/vidthumb.tmp." << rand() << ".png";

  vidthumb
    .arg("-o").arg(out.str())
    .arg(filename);

  if (vidthumb.exec() == 0)
  {
    std::cout.write(vidthumb.get_stdout().data(),
                    vidthumb.get_stdout().size());
    SoftwareSurfacePtr surface = PNG::load_from_file(out.str());
    remove(out.str().c_str());
    return surface;
  }
  else
  {
    throw std::runtime_error("VidThumb::load_from_file(): " + std::string(vidthumb.get_stderr().begin(), 
                                                                          vidthumb.get_stderr().end()));
  }  
}

/* EOF */
