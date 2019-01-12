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

#ifndef HEADER_GALAPIX_PLUGINS_JPEG_DECOMPRESSOR_HPP
#define HEADER_GALAPIX_PLUGINS_JPEG_DECOMPRESSOR_HPP

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#include "math/size.hpp"
#include "util/software_surface.hpp"

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

  Size read_size();
  SoftwareSurface read_image(int scale, Size* image_size);

private:
  [[noreturn]]
  static void fatal_error_handler(j_common_ptr cinfo);

private:
  JPEGDecompressor(const JPEGDecompressor&);
  JPEGDecompressor& operator=(const JPEGDecompressor&);
};

#endif

/* EOF */
