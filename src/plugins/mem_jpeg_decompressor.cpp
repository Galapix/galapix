// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "plugins/mem_jpeg_decompressor.hpp"

#include "plugins/jpeg_memory_src.hpp"

MemJPEGDecompressor::MemJPEGDecompressor(const uint8_t* data, size_t len)
{
  jpeg_memory_src(&m_cinfo, data, static_cast<int>(len));
}

MemJPEGDecompressor::~MemJPEGDecompressor()
{
}

/* EOF */
