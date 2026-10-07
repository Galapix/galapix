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

#include "plugins/jpeg_decompressor.hpp"

#include <assert.h>
#include <sstream>
#include <stdexcept>

namespace surf {

void
JPEGDecompressor::fatal_error_handler(j_common_ptr cinfo)
{
  longjmp(reinterpret_cast<ErrorMgr*>(cinfo->err)->setjmp_buffer, 1);
}

JPEGDecompressor::JPEGDecompressor() :
  m_cinfo(),
  m_err()
{
  jpeg_std_error(&m_err.pub);

  m_err.pub.error_exit = &JPEGDecompressor::fatal_error_handler;

  m_cinfo.err = &m_err.pub;

  jpeg_create_decompress(&m_cinfo);
}

JPEGDecompressor::~JPEGDecompressor()
{
  jpeg_destroy_decompress(&m_cinfo);
}

geom::isize
JPEGDecompressor::read_size()
{
  if (setjmp(m_err.setjmp_buffer)) {
    char buffer[JMSG_LENGTH_MAX];
    (m_cinfo.err->format_message)(reinterpret_cast<jpeg_common_struct*>(&m_cinfo), buffer);

    std::ostringstream out;
    out << "JPEG::read_size(): " /*<< filename << ": "*/ << buffer;
    throw std::runtime_error(out.str());
  }

  jpeg_read_header(&m_cinfo, /*require_image*/ FALSE);

  return geom::isize(static_cast<int>(m_cinfo.image_width),
                     static_cast<int>(m_cinfo.image_height));
}

PixelData<RGB8Pixel>
JPEGDecompressor::read_image(int scale, geom::isize* image_size)
{
  if (!(scale == 1 ||
        scale == 2 ||
        scale == 4 ||
        scale == 8))
  {
    throw std::invalid_argument("JPEGDecompressor::read_image: Invalid scale: " + std::to_string(scale));
  }

  // Everything with a destructor is declared before setjmp(), as
  // longjmp() would otherwise skip over it
  PixelData<RGB8Pixel> dst;
  std::vector<JSAMPLE const*> scanlines;
  std::vector<JSAMPLE> output_data;
  std::vector<JSAMPLE*> cmyk_scanlines;

  if (setjmp(m_err.setjmp_buffer)) {
    char buffer[JMSG_LENGTH_MAX];
    (m_cinfo.err->format_message)(reinterpret_cast<jpeg_common_struct*>(&m_cinfo), buffer);

    std::ostringstream out;
    out << "JPEG::read_image(): " /*<< filename << ": "*/ << buffer;
    throw std::runtime_error(out.str());
  }

  jpeg_read_header(&m_cinfo, /*require_image*/ FALSE);

  if (image_size) {
    *image_size = geom::isize(static_cast<int>(m_cinfo.image_width),
                              static_cast<int>(m_cinfo.image_height));
  }

  if (scale != 1) {// scale the image down by \a scale
    // by default all those values below are on 1
    m_cinfo.scale_num = 1;
    m_cinfo.scale_denom = static_cast<unsigned int>(scale);

    m_cinfo.do_fancy_upsampling = FALSE; /* TRUE=apply fancy upsampling */
    m_cinfo.do_block_smoothing  = FALSE; /* TRUE=apply interblock smoothing */
  }

  jpeg_start_decompress(&m_cinfo);

  dst = PixelData<RGB8Pixel>(geom::isize(static_cast<int>(m_cinfo.output_width),
                                         static_cast<int>(m_cinfo.output_height)));

  if (m_cinfo.out_color_space == JCS_RGB &&
      m_cinfo.output_components == 3) {
    scanlines.resize(m_cinfo.output_height);

    for (JDIMENSION y = 0; y < m_cinfo.output_height; ++y) {
      scanlines[y] = static_cast<JSAMPLE const*>(dst.get_row_data(static_cast<int>(y)));
    }

    while (m_cinfo.output_scanline < m_cinfo.output_height) {
      jpeg_read_scanlines(&m_cinfo, const_cast<JSAMPROW*>(&scanlines[m_cinfo.output_scanline]),
                          m_cinfo.output_height - m_cinfo.output_scanline);
    }
  } else if (m_cinfo.out_color_space == JCS_GRAYSCALE &&
             m_cinfo.output_components == 1) {
    scanlines.resize(m_cinfo.output_height);

    for (JDIMENSION y = 0; y < m_cinfo.output_height; ++y) {
      scanlines[y] = static_cast<JSAMPLE const*>(dst.get_row_data(static_cast<int>(y)));
    }

    while (m_cinfo.output_scanline < m_cinfo.output_height) {
      jpeg_read_scanlines(&m_cinfo, const_cast<JSAMPROW*>(&scanlines[m_cinfo.output_scanline]),
                          m_cinfo.output_height - m_cinfo.output_scanline);
    }

    // Expand the greyscale data to RGB
    // FIXME: Could be made faster if PixelData would support
    // other color formats
    for (int y = 0; y < dst.get_height(); ++y)
    {
      uint8_t* rowptr = static_cast<uint8_t*>(dst.get_row_data(y));
      for (int x = dst.get_width() - 1; x >= 0; --x)
      {
        rowptr[3*x+0] = rowptr[x];
        rowptr[3*x+1] = rowptr[x];
        rowptr[3*x+2] = rowptr[x];
      }
    }
  } else if (m_cinfo.out_color_space == JCS_CMYK &&
             m_cinfo.output_components == 4) {
    output_data.resize(static_cast<size_t>(m_cinfo.output_width) * m_cinfo.output_height *
                       static_cast<size_t>(m_cinfo.output_components));
    cmyk_scanlines.resize(m_cinfo.output_height);

    for(JDIMENSION y = 0; y < m_cinfo.output_height; ++y)
    {
      cmyk_scanlines[y] = &output_data[static_cast<size_t>(y) * m_cinfo.output_width * static_cast<size_t>(m_cinfo.output_components)];
    }

    while (m_cinfo.output_scanline < m_cinfo.output_height)
    {
      jpeg_read_scanlines(&m_cinfo, &cmyk_scanlines[m_cinfo.output_scanline],
                          m_cinfo.output_height - m_cinfo.output_scanline);
    }

    for(int y = 0; y < dst.get_height(); ++y)
    {
      uint8_t* jpegptr = &output_data[y * m_cinfo.output_width * m_cinfo.output_components];
      uint8_t* rowptr = static_cast<uint8_t*>(dst.get_row_data(y));
      for(int x = dst.get_width()-1; x >= 0; --x)
      {
        uint8_t const cmyk_c = jpegptr[4*x + 0];
        uint8_t const cmyk_m = jpegptr[4*x + 1];
        uint8_t const cmyk_y = jpegptr[4*x + 2];
        uint8_t const cmyk_k = jpegptr[4*x + 3];

        rowptr[3*x+0] = static_cast<uint8_t>((cmyk_c * cmyk_k) / 255);
        rowptr[3*x+1] = static_cast<uint8_t>((cmyk_m * cmyk_k) / 255);
        rowptr[3*x+2] = static_cast<uint8_t>((cmyk_y * cmyk_k) / 255);
      }
    }
  } else {
    std::ostringstream str;
    str << "JPEGDecompressor::read_image(): Unsupported colorspace: "
        << static_cast<int>(m_cinfo.out_color_space) << " components: " << m_cinfo.output_components;
    throw std::runtime_error(str.str());
  }

  return dst;
}

} // namespace surf

/* EOF */
