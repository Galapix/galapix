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

#ifndef HEADER_WINDSTILLE_MEM_JPEG_LOADER_HPP
#define HEADER_WINDSTILLE_MEM_JPEG_LOADER_HPP

#include "plugins/jpeg_loader.hpp"

class MemJPEGLoader : public JPEGLoader
{
private:
public:
  MemJPEGLoader(uint8_t* mem, int len);
  ~MemJPEGLoader();

private:
  MemJPEGLoader(const MemJPEGLoader&);
  MemJPEGLoader& operator=(const MemJPEGLoader&);
};

#endif

/* EOF */
