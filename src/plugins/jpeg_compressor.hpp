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

#ifndef HEADER_GALAPIX_PLUGINS_JPEG_COMPRESSOR_HPP
#define HEADER_GALAPIX_PLUGINS_JPEG_COMPRESSOR_HPP

#include <stdio.h>
#include <jpeglib.h>

#include "util/software_surface.hpp"

class JPEGCompressor
{
protected:
  struct jpeg_compress_struct m_cinfo;
  struct jpeg_error_mgr m_jerr;

protected:
  JPEGCompressor();

public:
  virtual ~JPEGCompressor();
  
  void save(SoftwareSurfacePtr surface_in, int quality);

private:
  JPEGCompressor(const JPEGCompressor&);
  JPEGCompressor& operator=(const JPEGCompressor&);
};

#endif

/* EOF */
