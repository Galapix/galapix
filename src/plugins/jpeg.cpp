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

#include <iostream>
#include <sstream>
#include <boost/bind.hpp>
#include <setjmp.h>

#include "math/size.hpp"
#include "plugins/jpeg_memory_dest.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/file_jpeg_loader.hpp"
#include "plugins/mem_jpeg_loader.hpp"

Size
JPEG::get_size(const std::string& filename)
{
  FileJPEGLoader loader(filename);
  loader.read_header();
  return loader.get_size();
}

SoftwareSurfaceHandle
JPEG::load_from_file(const std::string& filename, int scale)
{
  FileJPEGLoader loader(filename);
  return loader.read_image(scale);
}

SoftwareSurfaceHandle
JPEG::load_from_mem(uint8_t* mem, int len, int scale)
{
  MemJPEGLoader loader(mem, len);
  return loader.read_image(scale);
}

void
JPEG::save(const SoftwareSurfaceHandle& surface, int quality, const std::string& filename)
{
  FILE* out = fopen(filename.c_str(), "wb");
  if (!out)
  {
    throw std::runtime_error("JPEG:save: Couldn't open " + filename + " for writing");
  }
  else
  {
    JPEG::save(surface, 
               boost::bind(jpeg_stdio_dest, _1, out),
               quality);
                 
    fclose(out);
  }
}

BlobHandle
JPEG::save(const SoftwareSurfaceHandle& surface, int quality)
{
  std::vector<uint8_t> data;

  JPEG::save(surface, boost::bind(jpeg_memory_dest, _1, &data), quality);

  // FIXME: Unneeded copy of data
  return Blob::copy(data);
}


void
JPEG::save(const SoftwareSurfaceHandle& surface_in, 
           const boost::function<void (j_compress_ptr)>& setup_dest_mgr, 
           int quality)
{
  SoftwareSurfaceHandle surface = surface_in->to_rgb();

  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);
  
  // Setup output manager
  setup_dest_mgr(&cinfo);

  cinfo.image_width  = surface->get_width();
  cinfo.image_height = surface->get_height();

  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space   = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
 
  jpeg_start_compress(&cinfo, TRUE);

  boost::scoped_array<JSAMPROW> row_pointer(new JSAMPROW[surface->get_height()]);
  
  for(int y = 0; y < surface->get_height(); ++y)
    row_pointer[y] = static_cast<JSAMPLE*>(surface->get_row_data(y));

  while(cinfo.next_scanline < cinfo.image_height)
  {
    jpeg_write_scanlines(&cinfo, &row_pointer[cinfo.next_scanline], 
                         surface->get_height() - cinfo.next_scanline);
  }
  
  jpeg_finish_compress(&cinfo);
  
  jpeg_destroy_compress(&cinfo);
}
  
/* EOF */
