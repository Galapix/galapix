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

#ifndef HEADER_GALAPIX_PLUGINS_DDS_SURFACE_HPP
#define HEADER_GALAPIX_PLUGINS_DDS_SURFACE_HPP

#include <stdint.h>

#include <vector>
#include <fstream>
#include <stdexcept>
//#include "system.hpp"
//#include "util.hpp"
//#include "rgb.hpp"

#define DDPF_ALPHAPIXELS                0x00000001
#define DDPF_ALPHA                      0x00000002
#define DDPF_FOURCC                     0x00000004
#define DDPF_PALETTEINDEXED4            0x00000008
#define DDPF_PALETTEINDEXEDTO8          0x00000010
#define DDPF_PALETTEINDEXED8            0x00000020
#define DDPF_RGB                        0x00000040
#define DDPF_COMPRESSED                 0x00000080
#define DDPF_RGBTOYUV                   0x00000100
#define DDPF_YUV                        0x00000200
#define DDPF_ZBUFFER                    0x00000400
#define DDPF_PALETTEINDEXED1            0x00000800
#define DDPF_PALETTEINDEXED2            0x00001000
#define DDPF_ZPIXELS                    0x00002000
#define DDPF_STENCILBUFFER              0x00004000
#define DDPF_ALPHAPREMULT               0x00008000
#define DDPF_LUMINANCE                  0x00020000
#define DDPF_BUMPLUMINANCE              0x00040000
#define DDPF_BUMPDUDV                   0x00080000

namespace {

uint32_t read_uint32(std::istream& in)
{
  uint32_t v;
  in.read(reinterpret_cast<char*>(&v), sizeof(v));
  return v;
}

} // namespace

class DDSCapabilities
{
public:
  // DDS files should always include DDSCAPS_TEXTURE. If the file
  // contains mipmaps, DDSCAPS_MIPMAP should be set. For any .dds file
  // with more than one main surface, such as a mipmaps, cubic
  // environment map, or volume texture, DDSCAPS_COMPLEX should also
  // be set.
  uint32_t dwCaps1;

  // For cubic environment maps, DDSCAPS2_CUBEMAP should be included
  // as well as one or more faces of the map
  // (DDSCAPS2_CUBEMAP_POSITIVEX, DDSCAPS2_CUBEMAP_NEGATIVEX,
  // DDSCAPS2_CUBEMAP_POSITIVEY, DDSCAPS2_CUBEMAP_NEGATIVEY,
  // DDSCAPS2_CUBEMAP_POSITIVEZ, DDSCAPS2_CUBEMAP_NEGATIVEZ). For
  // volume textures, DDSCAPS2_VOLUME should be included.
  uint32_t dwCaps2;

  DDSCapabilities() :
    dwCaps1(0),
    dwCaps2(0)
  {
  }

  DDSCapabilities(std::istream& in) :
    dwCaps1(read_uint32(in)),
    dwCaps2(read_uint32(in))
  {
  }
};

class PixelFormat
{
public:
  // Size of structure. This member must be set to 32.
  uint32_t dwSize;

  // Flags to indicate valid fields. Uncompressed formats will usually
  // use DDPF_RGB to indicate an RGB format, while compressed formats
  // will use DDPF_FOURCC with a four-character code.
  uint32_t dwFlags;

  // This is the four-character code for compressed formats. dwFlags
  // should include DDPF_FOURCC in this case. For DXTn compression,
  // this is set to "DXT1", "DXT2", "DXT3", "DXT4", or "DXT5".
  uint32_t dwFourCC;

  // For RGB formats, this is the total number of bits in the
  // format. dwFlags should include DDPF_RGB in this case. This value
  // is usually 16, 24, or 32. For A8R8G8B8, this value would be 32.
  uint32_t dwRGBBitCount;

  // For RGB formats, these three fields contain the masks for the
  // red, green, and blue channels. For A8R8G8B8, these values would
  // be 0x00ff0000, 0x0000ff00, and 0x000000ff respectively.
  uint32_t dwRBitMask;
  uint32_t dwGBitMask;
  uint32_t dwBBitMask;

  // For RGB formats, this contains the mask for the alpha channel, if
  // any. dwFlags should include DDPF_ALPHAPIXELS in this case. For
  // A8R8G8B8, this value would be 0xff000000.
  uint32_t dwRGBAlphaBitMask;

  PixelFormat() :
    dwSize(),
    dwFlags(),
    dwFourCC(),
    dwRGBBitCount(),
    dwRBitMask(),
    dwGBitMask(),
    dwBBitMask(),
    dwRGBAlphaBitMask()
  {
  }

  PixelFormat(std::istream& in) :
    dwSize(),
    dwFlags(),
    dwFourCC(),
    dwRGBBitCount(),
    dwRBitMask(),
    dwGBitMask(),
    dwBBitMask(),
    dwRGBAlphaBitMask()
  {
    dwSize   = read_uint32(in);
    dwFlags  = read_uint32(in);
    dwFourCC = read_uint32(in);
    dwRGBBitCount = read_uint32(in);
    dwRBitMask = read_uint32(in);
    dwGBitMask = read_uint32(in);
    dwBBitMask = read_uint32(in);
    dwRGBAlphaBitMask = read_uint32(in);
  }
};

enum
{
  DDS_DXT1 = 827611204,
  DDS_DXT2 = 844388420,
  DDS_DXT3 = 861165636,
  DDS_DXT4 = 877942852,
  DDS_DXT5 = 894720068
};

enum DDSFlags
{
  DDSD_ALL          = 1047022,
  DDSD_ALPHABITDEPTH =    128,
  DDSD_BACKBUFFERCOUNT =   32,
  DDSD_CAPS         =       1,
  DDSD_CKDESTBLT    =   16384,
  DDSD_CKDESTOVERLAY =   8192,
  DDSD_CKSRCBLT     =   65536,
  DDSD_CKSRCOVERLAY =   32768,
  DDSD_HEIGHT       =       2,
  DDSD_LINEARSIZE   =  524288,
  DDSD_LPSURFACE    =    2048,
  DDSD_MIPMAPCOUNT  =  131072,
  DDSD_PITCH        =       8,
  DDSD_PIXELFORMAT  =    4096,
  DDSD_REFRESHRATE  =  262144,
  DDSD_TEXTURESTAGE = 1048576,
  DDSD_WIDTH        =       4,
  DDSD_ZBUFFERBITDEPTH =   64
};

class DDSSurface
{
public:
  uint32_t magic;

  // Size of structure. This member must be set to 124.
  uint32_t dwSize;

  // Flags to indicate valid fields. Always include DDSD_CAPS,
  // DDSD_PIXELFORMAT, DDSD_WIDTH, DDSD_HEIGHT.
  uint32_t flags;

  // Height of the main image in pixels
  uint32_t height;

  // Width of the main image in pixels
  uint32_t width;

  // For uncompressed formats, this is the number of bytes per scan
  // line (uint32_t> aligned) for the main image. dwFlags should include
  // DDSD_PITCH in this case. For compressed formats, this is the
  // total number of bytes for the main image. dwFlags should be
  // include DDSD_LINEARSIZE in this case.
  uint32_t dwPitchOrLinearSize;

  // For volume textures, this is the depth of the volume. dwFlags
  // should include DDSD_DEPTH in this case.
  uint32_t dwDepth;

  // For items with mipmap levels, this is the total number of levels
  // in the mipmap chain of the main image. dwFlags should include
  // DDSD_MIPMAPCOUNT in this case.
  uint32_t dwMipMapCount;

  // Unused
  uint32_t dwReserved1[11];

  // 32-byte value that specifies the pixel format structure.
  PixelFormat pixel_format;

  // 16-byte value that specifies the capabilities structure.
  DDSCapabilities ddsCaps;

  // Unused
  uint32_t dwReserved2;

private:
  // Contains the data of the image in RGBA format, only valid after calling read_data()
  std::vector<uint8_t> data;

public:
  DDSSurface(std::istream& in);

  uint8_t* get_data();

  int get_width() const;
  int get_height() const;
  size_t get_length() const;

private:
  /** Returns the image data in RGBA format */
  void read_data(std::istream& in);

  void decode_dxt1(unsigned char buf[8], unsigned char out[4*4*3]);
  void read_data_dxt1(std::istream& in);
  void read_data_dtx3(std::istream& in);
};

#endif

/* EOF */
