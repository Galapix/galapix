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

#ifdef HAVE_EXIF

#include "plugins/exif.hpp"

#include <libexif/exif-data.h>

namespace surf {
namespace exif {
namespace {

Transform get_orientation_exif(ExifData* ed)
{
  if (!ed)
  {
    return Transform::ROTATE_0;
  }
  else
  {
    ExifEntry* entry = exif_data_get_entry(ed, EXIF_TAG_ORIENTATION);

    if (!entry)
    {
      return Transform::ROTATE_0;
    }
    else
    {
      ExifByteOrder byte_order = exif_data_get_byte_order(ed);
      int orientation = exif_get_short(entry->data, byte_order);

      switch(orientation)
      {
        case 1: // The 0th row is at the visual top    of the image, and the 0th column is the visual left-hand side.
          return Transform::ROTATE_0;

        case 2: // The 0th row is at the visual top    of the image, and the 0th column is the visual right-hand side.
          return Transform::ROTATE_180_FLIP;

        case 3: // The 0th row is at the visual bottom of the image, and the 0th column is the visual right-hand side.
          return Transform::ROTATE_180;

        case 4: // The 0th row is at the visual bottom of the image, and the 0th column is the visual left-hand side.
          return Transform::ROTATE_0_FLIP;

        case 5: // The 0th row is the visual left-hand  side of the image, and the 0th column is the visual top.
          return Transform::ROTATE_270_FLIP;

        case 6: // The 0th row is the visual right-hand side of the image, and the 0th column is the visual top.
          return Transform::ROTATE_90;

        case 7: // The 0th row is the visual right-hand side of the image, and the 0th column is the visual bottom.
          return Transform::ROTATE_90_FLIP;

        case 8: // The 0th row is the visual left-hand  side of the image, and the 0th column is the visual bottom.
          return Transform::ROTATE_270;

        default:
          return Transform::ROTATE_0;
      }
    }
  }
}

} // namespace

Transform get_orientation(std::span<uint8_t const> data)
{
  ExifData* ed = exif_data_new_from_data(data.data(), static_cast<unsigned int>(data.size()));
  Transform orientation = get_orientation_exif(ed);
  exif_data_free(ed);
  return orientation;
}

Transform get_orientation(std::filesystem::path const& filename)
{
  ExifData* ed = exif_data_new_from_file(filename.string().c_str());
  Transform orientation = get_orientation_exif(ed);
  exif_data_free(ed);
  return orientation;
}

} // namespace exif
} // namespace surf

#endif

/* EOF */
