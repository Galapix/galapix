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

#include <iostream>
#include <GL/glew.h>
#include <GL/gl.h>
#include "math/rgb.hpp"
#include "SDL_syswm.h"
#include "math/rect.hpp"
#include "framebuffer.hpp"

SDL_SysWMinfo syswm;
SDL_Surface* Framebuffer::screen = 0;
Uint32 Framebuffer::flags = 0;
Size Framebuffer::desktop_resolution;

void
Framebuffer::set_video_mode(const Size& size, bool fullscreen)
{
  assert(screen == 0);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   5);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  5);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1); // vsync

  const SDL_VideoInfo* info = SDL_GetVideoInfo();
  desktop_resolution = Size(info->current_w, info->current_h);

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

  GLenum err = glewInit();
  if (GLEW_OK != err)
    {
      std::ostringstream str;
      str << "Error: " << glewGetErrorString(err) << std::endl;
      throw std::runtime_error(str.str());
    }
  
  if (!GLEW_ARB_texture_rectangle)
    {
      throw std::runtime_error("OpenGL ARB_texture_rectangle extension not found, but required");
    }

  SDL_WM_SetCaption("Griv 0.0.2", 0 /* icon */);
  SDL_EnableUNICODE(1);

  glViewport(0, 0, screen->w, screen->h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, screen->w, screen->h, 0.0, 1000.0, -1000.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //static const float cl_pixelcenter_constant = 0.375;
  //glTranslated(cl_pixelcenter_constant, cl_pixelcenter_constant, 0.0);
  
  SDL_VERSION(&syswm.version); // this is important!
  if (SDL_GetWMInfo(&syswm) == -1)
    {
      std::cout << "Couldn't get WM info " << std::endl;
    }
}

void
Framebuffer::toggle_fullscreen()
{
  Size res(800,600);
  if (flags & SDL_FULLSCREEN)
    {
      flags = SDL_OPENGL | SDL_RESIZABLE;
      res = Size(800, 600);
      std::cout << "Switching to fullscreen " 
                << res.width << "x" << res.height << std::endl;
    }
  else
    {
      flags = SDL_OPENGL | SDL_FULLSCREEN;
      res = desktop_resolution;
      std::cout << "Switching to desktop " 
                << res.width << "x" << res.height << std::endl;
    }
 
  screen = SDL_SetVideoMode(res.width, res.height, 0, flags); 
  glViewport(0, 0, screen->w, screen->h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, screen->w, screen->h, 0.0, 1000.0, -1000.0);
}

void
Framebuffer::resize(int w, int h)
{
  screen = SDL_SetVideoMode(w, h, 0, SDL_OPENGL | SDL_RESIZABLE);
  glViewport(0, 0, screen->w, screen->h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, screen->w, screen->h, 0.0, 1000.0, -1000.0);
}

void
Framebuffer::flip()
{
  //SDL_Flip(screen);
  SDL_GL_SwapBuffers();
}

void
Framebuffer::clear()
{
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //SDL_FillRect(Framebuffer::get_screen(), NULL, SDL_MapRGB(Framebuffer::get_screen()->format, 0, 0, 0));
}

void
Framebuffer::draw_rect(const Rectf& rect, const RGB& rgb)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
    
  glColor3ub(rgb.r, rgb.g, rgb.b);

  glBegin(GL_LINE_LOOP);
  glVertex2f(rect.left,  rect.top);
  glVertex2f(rect.right, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glVertex2f(rect.left,  rect.bottom);
  glEnd();
}

void
Framebuffer::fill_rect(const Rectf& rect, const RGB& rgb)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);

  glColor3ub(rgb.r, rgb.g, rgb.b);
  glBegin(GL_QUADS);
  glVertex2f(rect.left,  rect.top);
  glVertex2f(rect.right, rect.top);
  glVertex2f(rect.right, rect.bottom);
  glVertex2f(rect.left,  rect.bottom);
  glEnd();
}

void
Framebuffer::draw_grid(int num_cells)
{
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
 
  glBegin(GL_LINES);
  //  if (grid_color)
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
  //else
  //  glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

    int cell_width = Framebuffer::get_width()/num_cells;
    for(int x = 1; x < num_cells; ++x)
      {
        glVertex2f(x*cell_width, 0);
        glVertex2f(x*cell_width, Framebuffer::get_height());
      }

    int cell_height = Framebuffer::get_height()/num_cells;
    for(int y = 1; y < num_cells; ++y)
      {
        glVertex2f(0, y*cell_height);
        glVertex2f(Framebuffer::get_width(), y*cell_height);
      }

  glEnd();
}

/* EOF */
