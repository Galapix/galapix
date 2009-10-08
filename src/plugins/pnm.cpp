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

#include "plugins/pnm.hpp"

#include <stdexcept>

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
  bool eof() { return ptr >= (data+len); }
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

SoftwareSurfaceHandle
PNM::load_from_mem(const char* data, int len)
{
  PNMMemReader pnm(data, len);

  SoftwareSurfaceHandle surface = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT, pnm.get_size());
  const uint8_t* src_pixels = (uint8_t*)pnm.get_pixel_data();
  uint8_t* dst_pixels = surface->get_data();
  //std::cout << "MaxVal: " << pnm.get_maxval() << std::endl;
  assert(pnm.get_maxval() == 255);
  if (pnm.get_magic() == "P6") // RGB
    {
      for(int i = 0; i < surface->get_width() * surface->get_height(); ++i)
        {
          dst_pixels[3*i+0] = src_pixels[3*i+0];
          dst_pixels[3*i+1] = src_pixels[3*i+1];
          dst_pixels[3*i+2] = src_pixels[3*i+2];
        }
    }
  else if (pnm.get_magic() == "P5") // Grayscale
    {
      for(int i = 0; i < surface->get_width() * surface->get_height(); ++i)
        {
          dst_pixels[3*i+0] = src_pixels[i];
          dst_pixels[3*i+1] = src_pixels[i];
          dst_pixels[3*i+2] = src_pixels[i];
        }
    }
  else
    {
      throw std::runtime_error("Unhandled PNM format: '" + pnm.get_magic() + "'");
    }

  return surface;
}

#ifdef __TEST__

// g++ -Wall -Werror -ansi -pedantic blob.cpp pnm.cpp -o myexec -D__TEST__

#include <iostream>
#include "util/blob.hpp"

int main(int argc, char** argv)
{
  if (argc < 2)
    {
      std::cout << "Usage: " << argv[0] << " [PNMFiles]..." << std::endl;
    }
  else
    {
      for(int i = 1; i < argc; ++i)
        {
          BlobHandle blob = Blob::from_file(argv[i]);
          PNMMemReader reader((char*)blob.get_data(), blob.size());
        }
    }
}

#endif

/* EOF */
