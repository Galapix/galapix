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

#include <stdexcept>
#include "math/size.hpp"
#include "pnm.hpp"

class PNMMemReader
{
private:
  const char* data;
  int len;
  const char* ptr;

public:
  PNMMemReader(const char* data, int len)
    : data(data),
      len(len),
      ptr(data)
  {
  }

  // FIXME: Unfinished

  std::string get_token() 
  {
    const char* start = ptr;
    while(!isspace(advance()));
    return std::string(start, ptr);
  }

  void skip_whitespace()
  {
    while(isspace(advance()));
  }

  char advance()
  {
    if (ptr < data+len)
      {
        return *ptr++;
      }
    else
      {
        throw std::runtime_error("PNM: Unexpected end of file");
      }
  }
};

void
PNM::read_header(const char* data, int len,
                 PNM::Format& format, int& width, int& height, int& max_value)
{
  //format = get_token();
  //skip_whitespace();
  //width  = get_token();
  //skip_whitespace();
  //height = get_token();
  //skip_whitespace();
  //max_value = get_token();

  
  //else
    {
      throw std::runtime_error("Unhandled PNM format");
    }
}

SoftwareSurface
PNM::load_from_mem(const char* data, int len)
{
  //P6 # Converted by xcf2pnm 1.0.4
  //1239 1024
  //255
  //data...

  Format format;
  Size   size;
  int    max_value;

  read_header(data, len, format, size.width, size.height, max_value);

  SoftwareSurface surface(SoftwareSurface::RGB_FORMAT, size);
  switch(format)
    {
      case PNM_GRAYSCALE:
        
        break;

      case PNM_PIXMAP:
        
        break;
    }

  return surface;
}

/* EOF */
