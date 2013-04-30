/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GENERATOR_BLOB_ACCESSOR_HPP
#define HEADER_GALAPIX_GENERATOR_BLOB_ACCESSOR_HPP

#include <string>

#include "util/blob.hpp"

class BlobAccessor
{
private:
  std::string m_filename;
  BlobPtr m_blob;

public:
  BlobAccessor(const std::string& filename);
  BlobAccessor(BlobPtr blob);

  bool has_stdio_name() const;
  std::string get_stdio_name();

  bool has_blob() const;
  BlobPtr get_blob();
  
private:
  BlobAccessor(const BlobAccessor&);
  BlobAccessor& operator=(const BlobAccessor&);
};

#endif

/* EOF */
