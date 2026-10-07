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

#ifndef HEADER_SURF_PLUGINS_MEM_JPEG_DECOMPRESSOR_HPP
#define HEADER_SURF_PLUGINS_MEM_JPEG_DECOMPRESSOR_HPP

#include <surf/plugins/jpeg_decompressor.hpp>

#include <span>

namespace surf {

class MemJPEGDecompressor : public JPEGDecompressor
{
public:
  MemJPEGDecompressor(std::span<uint8_t const> data);
  ~MemJPEGDecompressor() override;

private:
  MemJPEGDecompressor(const MemJPEGDecompressor&);
  MemJPEGDecompressor& operator=(const MemJPEGDecompressor&);
};

} // namespace surf

#endif

/* EOF */
