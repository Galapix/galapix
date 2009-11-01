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

#ifndef HEADER_GALAPIX_PLUGINS_PNM_MEM_READER_HPP
#define HEADER_GALAPIX_PLUGINS_PNM_MEM_READER_HPP

#include "math/size.hpp"

class PNMMemReader
{
private:
  const char* data;
  int len;
  const char* ptr;
  int token_count;

  std::string magic;
  Size size;
  int maxval;
  const char* pixel_data;

private:
  bool eof() const { return ptr >= (data+len); }
  void forward()  { ptr += 1; }

  void parse()
  {
    token_count = 0;

    while(!eof())
    {
      if (*ptr == '#')
      {
        // skip comment
        forward();
        while(!eof() && *ptr != '\n')
          forward();
      }
      else if (isspace(*ptr))
      {
        // skip whitespace
        forward();
        while(!eof() && isspace(*ptr))
          forward();
      }
      else
      {
        const char* start = ptr;
        forward();
        while(!eof() && !isspace(*ptr) && *ptr != '#')
        {
          forward();
        }

        std::string token(start, ptr);
        switch(token_count)
        {
          case 0:
            magic = token;
            break;

          case 1:
            size.width = atoi(token.c_str());
            break;

          case 2:
            size.height = atoi(token.c_str());
            break;

          case 3:
            maxval = atoi(token.c_str());
            forward();
            pixel_data = ptr;
            ptr = data+len; // set ptr to EOF 
            break;
        }

        token_count += 1;
      }
    }
  }

public:
  PNMMemReader(const char* data_, int len_) :
    data(data_),
    len(len_),
    ptr(data_),
    token_count(),
    magic(),
    size(),
    maxval(),
    pixel_data()
  {
    parse();
  }

  std::string get_magic() const { return magic; }
  const char* get_pixel_data() const { return pixel_data; }
  Size get_size() const { return size; }
  int  get_maxval() const { return maxval; }

private:
  PNMMemReader(const PNMMemReader&);
  PNMMemReader& operator=(const PNMMemReader&);
};

#endif

/* EOF */
