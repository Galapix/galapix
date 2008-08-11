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

/* EOF */
