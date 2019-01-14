/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include "plugins/exif.hpp"

#include <libexif/exif-data.h>

namespace {

SoftwareSurface::Modifier get_orientation(ExifData* ed)
{
  if (!ed)
  {
    return SoftwareSurface::kRot0;
  }
  else
  {
    ExifEntry* entry = exif_data_get_entry(ed, EXIF_TAG_ORIENTATION);

    if (!entry)
    {
      return SoftwareSurface::kRot0;
    }
    else
    {
      ExifByteOrder byte_order = exif_data_get_byte_order(ed);
      int orientation = exif_get_short(entry->data, byte_order);

      switch(orientation)
      {
        case 1: // The 0th row is at the visual top    of the image, and the 0th column is the visual left-hand side.
          return SoftwareSurface::kRot0;

        case 2: // The 0th row is at the visual top    of the image, and the 0th column is the visual right-hand side.
          return SoftwareSurface::kRot180Flip;

        case 3: // The 0th row is at the visual bottom of the image, and the 0th column is the visual right-hand side.
          return SoftwareSurface::kRot180;

        case 4: // The 0th row is at the visual bottom of the image, and the 0th column is the visual left-hand side.
          return SoftwareSurface::kRot0Flip;

        case 5: // The 0th row is the visual left-hand  side of the image, and the 0th column is the visual top.
          return SoftwareSurface::kRot270Flip;

        case 6: // The 0th row is the visual right-hand side of the image, and the 0th column is the visual top.
          return SoftwareSurface::kRot90;

        case 7: // The 0th row is the visual right-hand side of the image, and the 0th column is the visual bottom.
          return SoftwareSurface::kRot90Flip;

        case 8: // The 0th row is the visual left-hand  side of the image, and the 0th column is the visual bottom.
          return SoftwareSurface::kRot270;

        default:
          return SoftwareSurface::kRot0;
      }
    }
  }
}

} // namespace

SoftwareSurface::Modifier
EXIF::get_orientation(const uint8_t* data, size_t len)
{
  ExifData* ed = exif_data_new_from_data(data, static_cast<unsigned int>(len));
  SoftwareSurface::Modifier orientation = ::get_orientation(ed);
  exif_data_free(ed);
  return orientation;
}

SoftwareSurface::Modifier
EXIF::get_orientation(const std::string& filename)
{
  ExifData* ed = exif_data_new_from_file(filename.c_str());
  SoftwareSurface::Modifier orientation = ::get_orientation(ed);
  exif_data_free(ed);
  return orientation;
}

/* EOF */
