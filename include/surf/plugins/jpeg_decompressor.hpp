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

#ifndef HEADER_SURF_PLUGINS_JPEG_DECOMPRESSOR_HPP
#define HEADER_SURF_PLUGINS_JPEG_DECOMPRESSOR_HPP

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <geom/size.hpp>

#include <surf/pixel_data.hpp>

namespace surf {

class JPEGDecompressor
{
protected:
  struct ErrorMgr
  {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
  };

protected:
  struct jpeg_decompress_struct  m_cinfo;
  struct ErrorMgr m_err;

protected:
  JPEGDecompressor();

public:
  virtual ~JPEGDecompressor();

  geom::isize read_size();
  PixelData<RGB8Pixel> read_image(int scale, geom::isize* image_size);

private:
  [[noreturn]]
  static void fatal_error_handler(j_common_ptr cinfo);

private:
  JPEGDecompressor(const JPEGDecompressor&);
  JPEGDecompressor& operator=(const JPEGDecompressor&);
};

} // namespace surf

#endif

/* EOF */
