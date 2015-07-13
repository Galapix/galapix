/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include <iostream>
#include <sstream>
#include <uitest/uitest.hpp>

#include "util/blob.hpp"
#include "plugins/pnm.hpp"
#include "plugins/png.hpp"

UITEST(PNM, test, "FILE...",
       "Load .pnm file")
{
  for(size_t i = 0; i < rest.size(); ++i)
  {
    BlobPtr blob = Blob::from_file(rest[i]);

    SoftwareSurfacePtr surface = PNM::load_from_mem(reinterpret_cast<const char*>(blob->get_data()), blob->size());

    std::ostringstream output_filename;
    output_filename << "/tmp/pnm_test" << i << ".png";
    PNG::save(surface, output_filename.str());
    std::cout << rest[i] << " -> " << output_filename.str() << std::endl;
  }
}

/* EOF */
