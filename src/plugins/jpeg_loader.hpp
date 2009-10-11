/*
**  Windstille - A Sci-Fi Action-Adventure Game
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_WINDSTILLE_JPEG_LOADER_HPP
#define HEADER_WINDSTILLE_JPEG_LOADER_HPP

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#include "math/size.hpp"
#include "util/software_surface.hpp"

class JPEGLoader
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
  JPEGLoader();

public:
  virtual ~JPEGLoader();
  
  Size get_size() const;

  void read_header();
  SoftwareSurfaceHandle read_image(int scale);

private:
  static void fatal_error_handler(j_common_ptr cinfo);
  
private:
  JPEGLoader(const JPEGLoader&);
  JPEGLoader& operator=(const JPEGLoader&);
};

#endif

/* EOF */
