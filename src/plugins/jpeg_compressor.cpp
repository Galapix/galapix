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

#include "plugins/jpeg_compressor.hpp"

JPEGCompressor::JPEGCompressor() :
  m_cinfo(),
  m_jerr()
{
  jpeg_std_error(&m_jerr);

  m_cinfo.err = &m_jerr;

  jpeg_create_compress(&m_cinfo);
}

JPEGCompressor::~JPEGCompressor()
{
  jpeg_destroy_compress(&m_cinfo);
}

void
JPEGCompressor::save(SoftwareSurface const& surface_in, int quality)
{
  SoftwareSurface surface = surface_in.to_rgb();
  PixelData const& pixel_data = surface.get_pixel_data();

  m_cinfo.image_width = static_cast<JDIMENSION>(pixel_data.get_width());
  m_cinfo.image_height = static_cast<JDIMENSION>(pixel_data.get_height());

  m_cinfo.input_components = 3; // # of color components per pixel
  m_cinfo.in_color_space = JCS_RGB; // colorspace of input image

  jpeg_set_defaults(&m_cinfo);
  jpeg_set_quality(&m_cinfo, quality, TRUE /* limit to baseline-JPEG values */);

  jpeg_start_compress(&m_cinfo, TRUE);

  std::vector<JSAMPROW> row_pointer(static_cast<size_t>(pixel_data.get_height()));
  for(int y = 0; y < pixel_data.get_height(); ++y)
  {
    row_pointer[static_cast<size_t>(y)] = const_cast<JSAMPLE*>(pixel_data.get_row_data(y));
  }

  while(m_cinfo.next_scanline < m_cinfo.image_height)
  {
    jpeg_write_scanlines(&m_cinfo, &row_pointer[m_cinfo.next_scanline],
                         static_cast<unsigned int>(pixel_data.get_height()) - m_cinfo.next_scanline);
  }

  jpeg_finish_compress(&m_cinfo);
}

/* EOF */
