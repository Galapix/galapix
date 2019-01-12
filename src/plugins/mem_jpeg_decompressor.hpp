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

#ifndef HEADER_GALAPIX_PLUGINS_MEM_JPEG_DECOMPRESSOR_HPP
#define HEADER_GALAPIX_PLUGINS_MEM_JPEG_DECOMPRESSOR_HPP

#include "plugins/jpeg_decompressor.hpp"

class MemJPEGDecompressor : public JPEGDecompressor
{
private:
public:
  MemJPEGDecompressor(const uint8_t* data, size_t len);
  ~MemJPEGDecompressor() override;

private:
  MemJPEGDecompressor(const MemJPEGDecompressor&);
  MemJPEGDecompressor& operator=(const MemJPEGDecompressor&);
};

#endif

/* EOF */
