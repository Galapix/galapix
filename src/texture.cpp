/* 
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
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL.h"

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

  TextureImpl(const SoftwareSurface& src, const Rect& srcrect)
    : size(srcrect.get_size())
  {
    glGenTextures(1, &handle);
  
    assert(src);
    
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, handle);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);

    glPixelStorei(GL_UNPACK_ALIGNMENT,  1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src.get_width());

    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGB,
                 size.width, size.height,
                 0, /* border */
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 src.get_data() + (src.get_pitch() * srcrect.top) + (srcrect.left*3));

    assert_gl("packing image texture");
    
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_S,     GL_CLAMP);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_WRAP_T,     GL_CLAMP);

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

Texture::Texture(const SoftwareSurface& src, const Rect& srcrect)
  : impl(new TextureImpl(src, srcrect))
{
}

Texture::~Texture()
{
}

void
Texture::bind()
{
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, impl->handle);
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
