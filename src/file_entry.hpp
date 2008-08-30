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

#ifndef HEADER_FILE_ENTRY_HPP
#define HEADER_FILE_ENTRY_HPP

#include <stdint.h>
#include <string>
#include "math/size.hpp"
#include "math/rgb.hpp"
#include "software_surface.hpp"

class FileEntry 
{
public:
  /** Unique id by which one can refer to this FileEntry, used in the
      'tile' table in the database */
  int fileid;

  /** The absolute filename of the image file */
  std::string filename; 

  /** MD5 Checksum of the image file 
      FIXME: currently not used */
  std::string md5;      

  /** MD5 Checksum of the image file 
      FIXME: currently not used */
  int         filesize; 

  /** Modification time of the image file
      FIXME: currently not used */
  uint32_t    mtime;

  /** The size of the image in pixels */
  Size size;

  /** The average color of the image, it can also be thought of as a
      1x1 thumbnail, it is used when drawing the place holder rect
      when no Tile is available */
  RGB color;

  /** A 8x8 thumbnail of the image, at this size the thumbnail doesn't
      consume more more diskspace then the filename, so it makes a
      good lower bound, it also the point at which the pixel data
      stored raw consumes less space then a JPEG compressed file */
  SoftwareSurface thumbnail;

  /** The maximum scale for which a tile is generated, any tile
      smaller then this will not be generated */
  int thumbnail_scale;
};

std::ostream& operator<<(std::ostream& os, const FileEntry& entry);

#endif

/* EOF */
