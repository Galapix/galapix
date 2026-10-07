// SPDX-FileCopyrightText: 2002-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/blitter.hpp>

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <surf/software_surface.hpp>

namespace wstdisplay {

void generate_border(surf::SoftwareSurface& surface,
                     int x_pos, int y_pos, int width, int height)
{
  assert(surface.get_format() == surf::PixelFormat::RGBA8);

  uint8_t* data = static_cast<uint8_t*>(surface.get_data());
  int pitch = surface.get_pitch();

  // duplicate the top line
  memcpy(data + (y_pos-1)*pitch + 4*x_pos,
         data + (y_pos)*pitch + 4*x_pos,
         4*width);

  // duplicate the bottom line
  memcpy(data + (y_pos+height)*pitch + 4*x_pos,
         data + (y_pos+height-1)*pitch + 4*x_pos,
         4*width);

  // duplicate left and right borders
  for(int y = y_pos-1; y < y_pos + height+1; ++y)
  {
    uint32_t* p = reinterpret_cast<uint32_t*> (data + (y*pitch + 4*(x_pos-1)));
    *p = *(p+1);
    p = reinterpret_cast<uint32_t*> (data + (y*pitch + 4*(x_pos + width)));
    *p = *(p-1);
  }
}

} // namespace wstdisplay

/* EOF */
