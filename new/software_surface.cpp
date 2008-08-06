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

#include <assert.h>
#include <iostream>
#include <stdexcept>
#include "FreeImage.h"

#include "math.hpp"
#include "math/rect.hpp"
#include "math/size.hpp"

#include "url.hpp"
#include "software_surface.hpp"

class SoftwareSurfaceImpl
{
public:
  FIBITMAP* bitmap;
  
  SoftwareSurfaceImpl(const std::string& filename)
  {
    FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filename.c_str(), 0);
    FIBITMAP* tmpimg = FreeImage_Load(format, filename.c_str());

    if (!tmpimg)
      {
        throw std::runtime_error("SoftwareSurface: Couldn't load " + filename);
      }
    else
      {
        { // Convert to 24bit
          bitmap = FreeImage_ConvertTo24Bits(tmpimg);
          FreeImage_Unload(tmpimg);
        }

        assert(FreeImage_GetImageType(bitmap) == FIT_BITMAP);
        assert(FreeImage_GetBPP(bitmap) == 24);
 
        // FIXME: Insert some assert() for colormask here

#if 0
        SDL_Surface* surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32,
                                                    FI_RGBA_RED_MASK,
                                                    FI_RGBA_GREEN_MASK,
                                                    FI_RGBA_BLUE_MASK,
                                                    FI_RGBA_ALPHA_MASK);
      
        SDL_LockSurface(surface);
        uint8_t* src = FreeImage_GetBits(img);
        uint8_t* dst = static_cast<uint8_t*>(surface->pixels);
        for(int y = 0; y < height; ++y)
          {
            memcpy(dst + surface->pitch*y, src + pitch*y, width*4);
          }
        SDL_UnlockSurface(surface);
#endif 
      }
  }

  SoftwareSurfaceImpl(FIBITMAP* bitmap_)
    : bitmap(bitmap_)
  {    
  }

  ~SoftwareSurfaceImpl() 
  {
    FreeImage_Unload(bitmap);
  }
};

SoftwareSurface::SoftwareSurface()
{
}

SoftwareSurface::SoftwareSurface(FIBITMAP* bitmap)
  : impl(new SoftwareSurfaceImpl(bitmap))
{  
}

SoftwareSurface::SoftwareSurface(const std::string& filename)
 : impl(new SoftwareSurfaceImpl(filename))
{
}

SoftwareSurface::~SoftwareSurface()
{
}

SoftwareSurface
SoftwareSurface::scale(const Size& size) const
{
  FIBITMAP* img = FreeImage_Rescale(impl->bitmap, size.width, size.height, FILTER_BOX);
  if (!img)
    throw std::runtime_error("SoftwareSurface: Couldn't scale image");
  else
    return SoftwareSurface(img);
}

SoftwareSurface
SoftwareSurface::crop(const Rect& rect) const
{
  FIBITMAP* img = FreeImage_Copy(impl->bitmap, 
                                 rect.left,  rect.top, 
                                 Math::min(rect.right,  get_width()),
                                 Math::min(rect.bottom, get_height()));
  if (!img)
    throw std::runtime_error("SoftwareSurface: Couldn't crop image");
  else
    return SoftwareSurface(img);
}

int
SoftwareSurface::get_width()  const
{
  return FreeImage_GetWidth(impl->bitmap); 
}

int
SoftwareSurface::get_height() const
{
  return FreeImage_GetHeight(impl->bitmap);    
}

int
SoftwareSurface::get_pitch()  const
{
  return FreeImage_GetPitch(impl->bitmap);  
}

void
SoftwareSurface::save(const std::string& filename) const
{
  std::cout << "Saving " << impl->bitmap << " " << get_width() << "x" << get_height() << " to " << filename << std::endl;
  if (!FreeImage_Save(FIF_JPEG, impl->bitmap, filename.c_str(), 0))
    {
      throw std::runtime_error("SoftwareSurface: Couldn't save image to " + filename);
    }
}

std::string
SoftwareSurface::get_data() const
{
  FIMEMORY* mem = FreeImage_OpenMemory();
  FreeImage_SaveToMemory(FIF_JPEG, impl->bitmap, mem, 0);

  BYTE* data;
  DWORD len;

  FreeImage_AcquireMemory(mem, &data, &len);

  std::string data_str(reinterpret_cast<char*>(data), len);

  FreeImage_CloseMemory(mem);

  return data_str;
}

// FreeImage_OpenMemory(BYTE *data FI_DEFAULT(0), DWORD size_in_bytes FI_DEFAULT(0));
// hmem = FreeImage_OpenMemory();
// FreeImage_SaveToMemory(fif, dib, hmem, 0);
// FreeImage_CloseMemory(hmem);
// FreeImage_AcquireMemory(FIMEMORY *stream, BYTE **data, DWORD* size_in_bytes);

  
/* EOF */
