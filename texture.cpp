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
7*/

#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>
#include <string.h>

#include "math/size.hpp"
#include "math/rect.hpp"
#include "framebuffer.hpp"
#include "software_surface.hpp"
#include "texture.hpp"

class TextureImpl
{
public:
  GLuint handle;
  Size   size;

  TextureImpl(const Size& size_,
              const SoftwareSurface& src, const Rect& srcrect)
    : size(size_)
  {
    glGenTextures(1, &handle);
  
    assert(src);
    
    glBindTexture(GL_TEXTURE_2D, handle);
    glEnable(GL_TEXTURE_2D);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 size.width, size.height,
                 0, /* border */
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 0 /* pixels */);

    assert_gl("packing image texture");

    
    if (0 && src.get_pitch() % 3 != 0)
      {
        std::cout << "pitch: " << src.get_pitch() << " width: " << src.get_width() << std::endl;
        assert(!"Align issue");
      }

    glPixelStorei(GL_UNPACK_ALIGNMENT,  8);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src.get_pitch()/3);
    
    // Upload the subimage
    glTexSubImage2D(GL_TEXTURE_2D, 0, 
                    0, 0, srcrect.get_width(), srcrect.get_height(), GL_BGR,
                    GL_UNSIGNED_BYTE, 
                    (Uint8*)src.get_data() + (src.get_pitch() * srcrect.top) + (srcrect.left * 3));

    assert_gl("creating texture");

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R,     GL_CLAMP);

    assert_gl("setting texture parameters");
  }

  ~TextureImpl()
  {
    glDeleteTextures(1, &handle);    
  }
};

Texture::Texture()
{
}

Texture::Texture(const Size& size,
                 const SoftwareSurface& src, const Rect& srcrect)
  : impl(new TextureImpl(size, src, srcrect))
{
}

Texture::~Texture()
{
}

void
Texture::bind()
{
  glBindTexture(GL_TEXTURE_2D, impl->handle);
}

int
Texture::get_width() const
{
  return impl->size.width;
}

int
Texture::get_height() const
{
  return impl->size.height;
}

/* EOF */
