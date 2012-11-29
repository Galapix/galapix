/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_INFO_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_INFO_HPP

#include "database/row_id.hpp"

class ImageInfo
{
private:
  RowId m_rowid;

public:
  ImageInfo();

  RowId get_rowid() const { return m_rowid; }

private:
  ImageInfo(const ImageInfo&);
  ImageInfo& operator=(const ImageInfo&);
};

#endif

/* EOF */
