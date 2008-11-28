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

#include <sstream>
#include <math.h>
#include "math.hpp"
#include <assert.h>
#include <iostream>
#include <stdexcept>

#include "framebuffer.hpp"
#include "sdl_framebuffer.hpp"

SDL_Surface* SDLFramebuffer::screen = 0;
Uint32 SDLFramebuffer::flags = 0;
Size SDLFramebuffer::desktop_resolution;
Size SDLFramebuffer::window_resolution;

void
SDLFramebuffer::set_video_mode(const Size& size, bool fullscreen, int anti_aliasing)
{
  assert(screen == 0);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

  if (anti_aliasing > 0)
    {
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
      SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, anti_aliasing);
    }
  
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1); // vsync

  const SDL_VideoInfo* info = SDL_GetVideoInfo();
  desktop_resolution = Size(info->current_w, info->current_h);
  window_resolution  = size;

  flags = SDL_RESIZABLE | SDL_OPENGL;

  if (fullscreen)
    {
      flags |= SDL_FULLSCREEN;
      screen = SDL_SetVideoMode(desktop_resolution.width, desktop_resolution.height, 0, flags);
    }
  else
    {
      screen = SDL_SetVideoMode(size.width, size.height, 0, flags);
    }

  if (screen == NULL) 
    {
      std::cout << "Unable to set video mode: " << SDL_GetError() << std::endl;
      exit(1);
    }

  SDL_WM_SetCaption("Galapix 0.0.4", 0 /* icon */);
  SDL_EnableUNICODE(1);

  Framebuffer::init();
  Framebuffer::reshape(Size(screen->w, screen->h));
}

void
SDLFramebuffer::toggle_fullscreen()
{
  Size res(800,600);
  if (flags & SDL_FULLSCREEN)
    {
      flags = SDL_OPENGL | SDL_RESIZABLE;
      res = window_resolution;
      std::cout << "Switching to fullscreen " 
                << res.width << "x" << res.height << std::endl;
    }
  else
    {
      window_resolution = Size(screen->w, screen->h);

      flags = SDL_OPENGL | SDL_FULLSCREEN;
      res = desktop_resolution;
      std::cout << "Switching to desktop " 
                << res.width << "x" << res.height << std::endl;
    }
 
  screen = SDL_SetVideoMode(res.width, res.height, 0, flags); 
  Framebuffer::reshape(Size(screen->w, screen->h));
}

void
SDLFramebuffer::reshape(const Size& size)
{
  screen = SDL_SetVideoMode(size.width, size.height, 0, SDL_OPENGL | SDL_RESIZABLE);
  Framebuffer::reshape(size);
}

void
SDLFramebuffer::flip()
{
  //SDL_Flip(screen);
  SDL_GL_SwapBuffers();
}

void
SDLFramebuffer::apply_gamma_ramp(float contrast, float brightness, float gamma)
{
  Uint16 tbl[256];
  for(int i = 0; i < 256; ++i)
    {
      float c = i/255.0f;
      c = c + brightness;
      c = (c * contrast) - 0.5f * (contrast - 1.0f);
      c = powf(c, 1.0f/gamma);
      
      tbl[i] = Math::clamp(0, (int)(c*65535.0f), 65535);
    }
  
  SDL_SetGammaRamp(tbl, tbl, tbl);
}

/* EOF */
