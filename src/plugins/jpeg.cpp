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
#include "plugins/jpeg_memory_src.hpp"
#include "plugins/jpeg_memory_dest.hpp"
#include "plugins/jpeg.hpp"

jmp_buf setjmp_buffer;

void fatal_error_handler(j_common_ptr /*cinfo*/)
{
  longjmp(setjmp_buffer, 1);
}

bool
JPEG::get_size(const std::string& filename, Size& size)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    {
      // throw std::runtime_error("JPEG::get_size: Couldn't open " + filename);
      return false;
    }
  else
    {
      struct jpeg_decompress_struct  cinfo;
      struct jpeg_error_mgr jerr;

      cinfo.err = jpeg_std_error(&jerr);
      cinfo.err->error_exit = &fatal_error_handler;
      jpeg_create_decompress(&cinfo);
      jpeg_stdio_src(&cinfo, in);

      if (setjmp(setjmp_buffer))
        {
          std::cout << "Some jpeg error: " << filename << std::endl;
          return false;
        }
      else
        {
          jpeg_read_header(&cinfo, FALSE);

          size.width  = cinfo.image_width;
          size.height = cinfo.image_height;

          jpeg_destroy_decompress(&cinfo);

          fclose(in);

          return true;
        }
    }
}

SoftwareSurfaceHandle
JPEG::load(const boost::function<void (j_decompress_ptr)>& setup_src_mgr,
           int scale)
{
  assert(scale == 1 ||
         scale == 2 ||
         scale == 4 ||
         scale == 8);

  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = &fatal_error_handler;

  jpeg_create_decompress(&cinfo);

  // Setup the JPEG source (stdio or mem)
  setup_src_mgr(&cinfo);

  if (setjmp(setjmp_buffer))
    {
      throw std::runtime_error("JPEG::load: ERROR");
    }

  jpeg_read_header(&cinfo, FALSE);

  if (scale != 1)
    { // scale the image down by scale

      // by default all those values below are on 1
      cinfo.scale_num           = 1;
      cinfo.scale_denom         = scale;
   
      cinfo.do_fancy_upsampling = FALSE; /* TRUE=apply fancy upsampling */
      cinfo.do_block_smoothing  = FALSE; /* TRUE=apply interblock smoothing */
    }

  jpeg_start_decompress(&cinfo);

  SoftwareSurfaceHandle surface = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT,
                                                          Size(cinfo.output_width,
                                                               cinfo.output_height));
  
  if (cinfo.output_components == 3)
    { // RGB Image
      boost::scoped_array<JSAMPLE*> scanlines(new JSAMPLE*[cinfo.output_height]);

      for(JDIMENSION y = 0; y < cinfo.output_height; ++y)
        scanlines[y] = surface->get_row_data(y);

      while (cinfo.output_scanline < cinfo.output_height) 
        {
          jpeg_read_scanlines(&cinfo, &scanlines[cinfo.output_scanline], 
                              cinfo.output_height - cinfo.output_scanline);
        }
    }
  else if (cinfo.output_components == 1)
    { // Greyscale Image
      boost::scoped_array<JSAMPLE*> scanlines(new JSAMPLE*[cinfo.output_height]);

      for(JDIMENSION y = 0; y < cinfo.output_height; ++y)
        scanlines[y] = surface->get_row_data(y);

      while (cinfo.output_scanline < cinfo.output_height) 
        {
          jpeg_read_scanlines(&cinfo, &scanlines[cinfo.output_scanline], 
                              cinfo.output_height - cinfo.output_scanline);
        }

      // Expand the greyscale data to RGB
      // FIXME: Could be made faster if SoftwareSurface would support
      // other color formats
      for(int y = 0; y < surface->get_height(); ++y)
        {
          uint8_t* rowptr = surface->get_row_data(y);
          for(int x = surface->get_width()-1; x >= 0; --x)
            {
              rowptr[3*x+0] = rowptr[x];
              rowptr[3*x+1] = rowptr[x];
              rowptr[3*x+2] = rowptr[x];
            }
        }
    }
  else
    {
      std::ostringstream str;
      str << "JPEG: Unsupported color depth: " << cinfo.output_components;
      throw std::runtime_error(str.str());
    }

  jpeg_destroy_decompress(&cinfo);

  return surface;
}  

SoftwareSurfaceHandle
JPEG::load_from_file(const std::string& filename, int scale)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    {
      throw std::runtime_error("JPEG::load_from_file: Couldn't open " + filename);
    }
  else
    {
      SoftwareSurfaceHandle surface = JPEG::load(boost::bind(jpeg_stdio_src, _1, in),
                                                 scale);

      fclose(in); 

      return surface;
    }
}

SoftwareSurfaceHandle
JPEG::load_from_mem(uint8_t* mem, int len, int scale)
{
  return JPEG::load(boost::bind(jpeg_memory_src, _1, mem, len), scale);
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
