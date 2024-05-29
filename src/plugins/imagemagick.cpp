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

#include "plugins/imagemagick.hpp"

#include <assert.h>
#include <stdexcept>
#include <Magick++.h>
#include <iostream>
#include <algorithm>
#include <list>

#include "util/url.hpp"
#include "math/size.hpp"

bool
Imagemagick::get_size(const std::string& filename, Size& size)
{
  try 
  {
    Magick::Image image(filename);
  
    size.width  = image.columns();
    size.height = image.rows();

    return true;
  } 
  catch(std::exception& err) 
  {
    std::cout << "Imagemagick: " << filename << ": " << err.what() << std::endl;
    return false;
  }
}

std::vector<std::string>
Imagemagick::get_supported_extensions()
{
  if (false)
  {
    /* Generating the list automatic doesn't work, as there ends up to
       be to much weird stuff in it (txt, avi, mpeg, etc.) that causes
       trouble */
    std::list<Magick::CoderInfo> coderList;

    Magick::coderInfoList(&coderList,
                          Magick::CoderInfo::TrueMatch, // Match readable formats
                          Magick::CoderInfo::AnyMatch,  // Don't care about writable formats
                          Magick::CoderInfo::AnyMatch); // Don't care about multi-frame support

    std::vector<std::string> lst;

    for(std::list<Magick::CoderInfo>::iterator entry = coderList.begin();
        entry != coderList.end();
        ++entry)
    {
      std::string data = entry->name();
      std::transform(data.begin(), data.end(), data.begin(), ::tolower);
      lst.push_back(data);
    }

    return lst;
  }
  else
  {
    std::vector<std::string> lst;

    lst.push_back("gif");
    lst.push_back("tga");
    lst.push_back("rgb");
    lst.push_back("bmp");
    lst.push_back("tiff");
    lst.push_back("tif");
    lst.push_back("pbm");
    lst.push_back("ppm");
    lst.push_back("pgm");
    lst.push_back("webp");

    return lst;
  }
}

static 
SoftwareSurfacePtr
MagickImage2SoftwareSurface(const Magick::Image& image)
{
  SoftwareSurfacePtr surface;

  int width  = image.columns();
  int height = image.rows();
  int shift = image.depth() - 8;

  if (image.alpha())
  {
    surface = SoftwareSurface::create(SoftwareSurface::RGBA_FORMAT, 
                                      Size(width, height));

    for(int y = 0; y < height; ++y)
    {
      using Magick::Quantum; // FIXME: Seriously? There must be a better way to get QuantumRange to work
      Magick::Quantum const* src_pixel = image.getConstPixels(0, y, width, 1);
      uint8_t* dst_pixels = surface->get_row_data(y);

      for(int x = 0; x < width; ++x)
      {
        dst_pixels[4*x + 0] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelRed(image.constImage(), src_pixel) / QuantumRange);
        dst_pixels[4*x + 1] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelGreen(image.constImage(), src_pixel) / QuantumRange);
        dst_pixels[4*x + 2] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelBlue(image.constImage(), src_pixel) / QuantumRange);
        dst_pixels[4*x + 3] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelAlpha(image.constImage(), src_pixel) / QuantumRange);

        src_pixel += image.channels();
      }
    }
  }
  else
  {
    surface = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT, 
                                      Size(width, height));
    for(int y = 0; y < height; ++y)
    {
      using Magick::Quantum;
      Magick::Quantum const* src_pixel = image.getConstPixels(0, y, width, 1);
      uint8_t* dst_pixels = surface->get_row_data(y);

      for(int x = 0; x < width; ++x)
      {
        dst_pixels[3*x + 0] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelRed(image.constImage(), src_pixel) / QuantumRange);
        dst_pixels[3*x + 1] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelGreen(image.constImage(), src_pixel) / QuantumRange);
        dst_pixels[3*x + 2] = static_cast<uint8_t>(255.0f * MagickCore::GetPixelBlue(image.constImage(), src_pixel) / QuantumRange);

        src_pixel += image.channels();
      }
    }
  }  

  return surface;
}

SoftwareSurfacePtr
Imagemagick::load_from_mem(void* data, int len)
{
  // FIXME: Magick::Blob creates an unneeded copy of the data
  return MagickImage2SoftwareSurface(Magick::Image(Magick::Blob(data, len))); 
}

SoftwareSurfacePtr
Imagemagick::load_from_file(const std::string& filename)
{
  return MagickImage2SoftwareSurface(Magick::Image(filename));
}

/* EOF */
