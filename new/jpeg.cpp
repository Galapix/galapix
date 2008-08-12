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
#include <stdexcept>
#include <jpeglib.h>
#include <setjmp.h>
#include "jpeg.hpp"

jmp_buf setjmp_buffer;

void fatal_error_handler(j_common_ptr cinfo)
{
  std::cout << "Some jpeg error: " << std::endl;
  longjmp(setjmp_buffer, 1);
}

void
JPEG::get_size(const std::string& filename, int& w, int& h)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    throw std::runtime_error("JPEG::get_size: Couldn't open " + filename);

  struct jpeg_decompress_struct  jinfo;
  struct jpeg_error_mgr jerr;

  jinfo.err = jpeg_std_error(&jerr);
  jinfo.err->error_exit = &fatal_error_handler;
  jpeg_create_decompress(&jinfo);
  jpeg_stdio_src(&jinfo, in);

  if (setjmp(setjmp_buffer))
    {
      throw std::runtime_error("JPEG::get_size: ERROR: Couldn't open " + filename);
    }

  jpeg_read_header(&jinfo, FALSE);

  w = jinfo.image_width;
  h = jinfo.image_height;

  jpeg_destroy_decompress(&jinfo);

  fclose(in);
}

void
JPEG::load(const std::string& filename)
{
  FILE* in = fopen(filename.c_str(), "rb");
  if (!in)
    throw std::runtime_error("JPEG::get_size: Couldn't open " + filename);

  struct jpeg_decompress_struct  jinfo;
  struct jpeg_error_mgr jerr;

  jinfo.err = jpeg_std_error(&jerr);
  jinfo.err->error_exit = &fatal_error_handler;

  jpeg_create_decompress(&jinfo);
  jpeg_stdio_src(&jinfo, in);

  if (setjmp(setjmp_buffer))
    {
      throw std::runtime_error("JPEG::get_size: ERROR: Couldn't open " + filename);
    }

  jpeg_read_header(&jinfo, FALSE);

  jpeg_start_decompress(&cinfo);

  int width  = jinfo.image_width;
  int height = jinfo.image_height;
  int pitch  = cinfo.output_width * cinfo.output_components;
  
  if (cinfo.output_components == 3)
    { // RGB Image
      uint8_t buffer[pitch];

      while (cinfo.output_scanline < cinfo.output_height)
        {
          jpeg_read_scanlines(&cinfo, buffer, 1);
		
          for(unsigned int i=0; i < get_pitch(); i += 3)
            {
              buffer[3*i + 0];
              buffer[3*i + 1];
              buffer[3*i + 2];
            }
        }
    }
  else if (cinfo.output_components == 1)
    { // Greyscale Image
      while (cinfo.output_scanline < cinfo.output_height) 
        {
          jpeg_read_scanlines(&cinfo, buffer, 1);
			
          for(int i=0; i < width; i += 1)
            {
              buffer[0][i];
              buffer[0][i];
              buffer[0][i];
            }			
        }
    }
  else
    {
      throw CL_Error(CL_String::format("CL_JPEGProvider: Unsupported color completion: %1",  cinfo.output_components));
    }

  jpeg_destroy_decompress(&jinfo);

  fclose(in); 
}

/* EOF */
