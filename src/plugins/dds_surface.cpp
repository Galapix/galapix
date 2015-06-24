/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmail.com>
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

#include "plugins/dds_surface.hpp"

#include <sstream>

#include "plugins/dds_rgb.hpp"
#include "util/raise_exception.hpp"

DDSSurface::DDSSurface(std::istream& in) :
  magic(),
  dwSize(),
  flags(),
  height(),
  width(),
  dwPitchOrLinearSize(),
  dwDepth(),
  dwMipMapCount(),
  dwReserved1(),
  pixel_format(),
  ddsCaps(),
  dwReserved2(),
  data()
{
  magic  = read_uint32(in);
  dwSize = read_uint32(in);
  flags  = read_uint32(in);
  height = read_uint32(in);
  width  = read_uint32(in);
  dwPitchOrLinearSize = read_uint32(in);
  dwDepth = read_uint32(in);
  dwMipMapCount = read_uint32(in);

  for(int i = 0; i < 11; ++i)
    dwReserved1[i] = read_uint32(in);

  pixel_format = PixelFormat(in);

  ddsCaps = DDSCapabilities(in);

  dwReserved2 = read_uint32(in);

  read_data(in);
}

int
DDSSurface::get_width() const
{
  return static_cast<int>(width);
}

int
DDSSurface::get_height() const
{
  return static_cast<int>(height);
}

size_t
DDSSurface::get_length() const
{
  return data.size();
}

uint8_t*
DDSSurface::get_data()
{
  return &*data.begin();
}

/** Returns the image data in RGBA format */
void
DDSSurface::read_data(std::istream& in)
{
  // Read pixel data
  if (flags & DDPF_FOURCC)
  {
    switch (pixel_format.dwFourCC)
    {
      case DDS_DXT1:
        read_data_dxt1(in);
        break;

      case DDS_DXT2:
        raise_runtime_error("DXT2 Format not supported");
        break;

      case DDS_DXT3:
        read_data_dtx3(in);
        break;

      case DDS_DXT4:
        raise_runtime_error("DXT4 Format not supported");
        break;

      case DDS_DXT5:
        read_data_dtx3(in);
        break;

      case 0:
        // HU?
        read_data_dxt1(in);
        break;

      default:
        {
          std::ostringstream str;
          str << "Format unknown: " << pixel_format.dwFourCC << " " << std::string((char*)&pixel_format.dwFourCC, 4);
          raise_runtime_error(str.str());
        }
    }
  }
  else
  {
  }
}

void
DDSSurface::decode_dxt1(unsigned char buf[8], unsigned char out[4*4*3])
{
  unsigned short color0 = static_cast<unsigned short>(buf[0] | (buf[1] << 8));
  unsigned short color1 = static_cast<unsigned short>(buf[2] | (buf[3] << 8));

  unsigned int bits = buf[4] + 256 * (buf[5] + 256 * (buf[6] + 256 * buf[7]));

  DDSRGB rgb[4];

  if (color0 > color1)
  {
    rgb[0] = color0;
    rgb[1] = color1;
    rgb[2] = (2*rgb[0] + rgb[1])/3;
    rgb[3] = (rgb[0] + 2*rgb[1])/3;
  }
  else
  {
    rgb[0] = color0;
    rgb[1] = color1;
    rgb[2] = (rgb[0] + rgb[1])/2;
    rgb[3] = DDSRGB(0);
  }

  for(int y1 = 0; y1 < 4; ++y1)
    for(int x1 = 0; x1 < 4; ++x1)
    {
      unsigned int idx = ((bits >> (2*(4*(y1)+(x1)))) & 0x3);

      out[3*4*y1 + 3*x1 + 2] = static_cast<uint8_t>(rgb[idx].r);
      out[3*4*y1 + 3*x1 + 1] = static_cast<uint8_t>(rgb[idx].g);
      out[3*4*y1 + 3*x1 + 0] = static_cast<uint8_t>(rgb[idx].b);
    }
}

void
DDSSurface::read_data_dxt1(std::istream& in)
{
  unsigned char buf[8];
  unsigned char out[4*4*3];

  data.resize(width * height * 4);
  for(unsigned int y = 0; y < height; y += 4)
    for(unsigned int x = 0; x < width; x += 4)
    {
      in.read((char*)buf, 8);
      decode_dxt1(buf, out);

      for(unsigned int y1 = 0; y1 < 4; y1 += 1)
        for(unsigned int x1 = 0; x1 < 4; x1 += 1)
        {
          data[4*(y+y1)*width + 4*(x+x1) + 0] = out[3*4*y1 + 3*x1 + 0];
          data[4*(y+y1)*width + 4*(x+x1) + 1] = out[3*4*y1 + 3*x1 + 1];
          data[4*(y+y1)*width + 4*(x+x1) + 2] = out[3*4*y1 + 3*x1 + 2];
          data[4*(y+y1)*width + 4*(x+x1) + 3] = 255;
        }
    }
}

void
DDSSurface::read_data_dtx3(std::istream& in)
{
  unsigned char buf[8];
  unsigned char out[4*4*3];

  data.resize(width * height * 4);
  for(unsigned int y = 0; y < height; y += 4)
    for(unsigned int x = 0; x < width; x += 4)
    {
      in.read((char*)buf, 8);
      decode_dxt1(buf, out);

      // Skip alpha information
      in.read((char*)buf, 8);

      for(unsigned int y1 = 0; y1 < 4; y1 += 1)
        for(unsigned int x1 = 0; x1 < 4; x1 += 1)
        {
          data[4*(y+y1)*width + 4*(x+x1) + 0] = out[3*4*y1 + 3*x1 + 0];
          data[4*(y+y1)*width + 4*(x+x1) + 1] = out[3*4*y1 + 3*x1 + 1];
          data[4*(y+y1)*width + 4*(x+x1) + 2] = out[3*4*y1 + 3*x1 + 2];
          data[4*(y+y1)*width + 4*(x+x1) + 3] = 255;
        }
    }
}

/* EOF */
