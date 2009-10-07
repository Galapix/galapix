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

#include "util/url.hpp"
#include "util/exec.hpp"
#include "plugins/png.hpp"

SoftwareSurface
RSVG::load_from_url(const URL& url)
{
  if (url.has_stdio_name())
    {
      return load_from_file(url.get_stdio_name());
    }
  else
    {
      assert(!"RSVG: Not supported");
      return SoftwareSurface();
    }
 
}

SoftwareSurface
RSVG::load_from_file(const std::string& filename)
{
  Exec rsvg("rsvg");

  rsvg.arg("--format").arg("png");
  rsvg.arg(filename);
  rsvg.arg("/dev/stdout");

  if (rsvg.exec() == 0)
    {
      BlobHandle blob = Blob::copy(&*rsvg.get_stdout().begin(), rsvg.get_stdout().size());
      SoftwareSurface surface = PNG::load_from_mem(blob->get_data(), blob->size());
      return surface;
    }
  else
    {
      throw std::runtime_error("RSVG: " + std::string(rsvg.get_stderr().begin(), rsvg.get_stderr().end()));
    }
}

/* EOF */
