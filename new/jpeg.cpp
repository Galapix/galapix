/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <jpeglib.h>
#include <setjmp.h>
#include "math/size.hpp"
#include "jpeg_memory_src.hpp"
#include "jpeg.hpp"

jmp_buf setjmp_buffer;

void fatal_error_handler(j_common_ptr cinfo)
{
  std::cout << "Some jpeg error: " << std::endl;
  longjmp(setjmp_buffer, 1);
}

void
JPEG::get_size(const std::string& filename, Size& size)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    throw std::runtime_error("JPEG::get_size: Couldn't open " + filename);

  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = &fatal_error_handler;
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, in);

  if (setjmp(setjmp_buffer))
    {
      throw std::runtime_error("JPEG::get_size: ERROR: Couldn't open " + filename);
    }

  jpeg_read_header(&cinfo, FALSE);

  size.width  = cinfo.image_width;
  size.height = cinfo.image_height;

  jpeg_destroy_decompress(&cinfo);

  fclose(in);
}

SoftwareSurface
JPEG::load(const std::string& filename)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    throw std::runtime_error("JPEG::get_size: Couldn't open " + filename);

  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = &fatal_error_handler;

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, in);

  if (setjmp(setjmp_buffer))
    {
      throw std::runtime_error("JPEG::get_size: ERROR: Couldn't open " + filename);
    }

  jpeg_read_header(&cinfo, FALSE);

  jpeg_start_decompress(&cinfo);

  SoftwareSurface surface(Size(cinfo.image_width, 
                               cinfo.image_height));
  
  if (cinfo.output_components == 3)
    { // RGB Image
      JSAMPLE* scanlines[cinfo.image_height];

      uint8_t* pixels = static_cast<uint8_t*>(surface.get_data());
      int      pitch  = surface.get_pitch();

      for(JDIMENSION y = 0; y < cinfo.image_height; ++y)
        scanlines[y] = &pixels[y * pitch];

      jpeg_read_scanlines(&cinfo, scanlines, cinfo.image_height);
    }
  else if (cinfo.output_components == 1)
    { // Greyscale Image
      
    }
  else
    {
      std::ostringstream str;
      str << "JPEG: Unsupported color depth: " << cinfo.output_components;
      throw std::runtime_error(str.str());
    }

  jpeg_destroy_decompress(&cinfo);

  fclose(in); 

  return surface;
}

SoftwareSurface
JPEG::load(uint8_t* mem, int len)
{
  //std::cout << "JPEG::load(" << static_cast<void*>(mem) << ", " << len << ")" << std::endl;
  
  // -- Setup the read source
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  cinfo.err->error_exit = &fatal_error_handler;

  jpeg_create_decompress(&cinfo);
  jpeg_memory_src(&cinfo, mem, len);

  if (setjmp(setjmp_buffer)) // FIXME: Is that error handling correct that way?
    {
      throw std::runtime_error("JPEG::load");
    }

  // -- Start Reading
  jpeg_read_header(&cinfo, FALSE);
  jpeg_start_decompress(&cinfo);

  // -- Copy the scanlines to a SoftwareSurface
  SoftwareSurface surface(Size(cinfo.image_width, 
                               cinfo.image_height));
  
  //std::cout << surface.get_width() << "x" << surface.get_height() << std::endl;

  if (cinfo.output_components == 3) // RGB Image
    { 
      JSAMPLE* scanlines[cinfo.image_height];

      for(JDIMENSION y = 0; y < cinfo.image_height; ++y)
        scanlines[y] = surface.get_row_data(y);

      while (cinfo.output_scanline < cinfo.output_height) 
        {
          jpeg_read_scanlines(&cinfo, &scanlines[cinfo.output_scanline], 
                              cinfo.image_height - cinfo.output_scanline);
        }
    }
  else if (cinfo.output_components == 1) // Greyscale Image
    { 
      assert(!"JPEG::load: grayscale handling not implemented");
    }
  else
    {
      std::ostringstream str;
      str << "JPEG: Unsupported color depth: " << cinfo.output_components;
      throw std::runtime_error(str.str());
    }

  // -- Cleanup
  jpeg_destroy_decompress(&cinfo);

  return surface; 
}

void
JPEG::save(SoftwareSurface& surface, int quality, const std::string& filename)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);

  //jpeg_stdio_dest(&cinfo, outfile);

  cinfo.image_width  = surface.get_width();
  cinfo.image_height = surface.get_height();

  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space   = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);
 
  jpeg_start_compress(&cinfo, TRUE);

  JSAMPROW row_pointer[surface.get_height()];
  
  for(int y = 0; y < surface.get_height(); ++y)
    row_pointer[y] = static_cast<JSAMPLE*>(surface.get_row_data(y));

  while(cinfo.next_scanline < cinfo.image_height)
    {
      jpeg_write_scanlines(&cinfo, row_pointer, surface.get_height());
    }
  
  jpeg_finish_compress(&cinfo);
  
  jpeg_destroy_compress(&cinfo);
}  

/* EOF */
