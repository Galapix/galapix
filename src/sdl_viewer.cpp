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

#include <boost/bind.hpp>
#include "../config.h"
#include "math/rgba.hpp"
#include "file_entry.hpp"
#include "workspace.hpp"
#include "sdl_framebuffer.hpp"
#include "viewer.hpp"
#include "sdl_viewer.hpp"
#include "tile_generator_thread.hpp"
#include "space_navigator.hpp"
#include "database_thread.hpp"

#ifdef HAVE_SPACE_NAVIGATOR
#  include <spnav.h>
#endif

SDLViewer* SDLViewer::current_ = 0;

SDLViewer::SDLViewer(const Size& geometry, bool fullscreen, int  anti_aliasing)
  : geometry(geometry),
    fullscreen(fullscreen),
    anti_aliasing(anti_aliasing),
    quit(false),
    spnav_allow_rotate(false)
{
  current_ = this;
}

SDLViewer::~SDLViewer()
{
}

void
SDLViewer::receive_file(const FileEntry& entry)
{
  file_queue.push(entry);
}

void
SDLViewer::process_event(const SDL_Event& event)
{
  //Uint8* keystates = SDL_GetKeyState(NULL);

  switch(event.type)
    {
      case SDL_USEREVENT:
        if (event.user.code == 0)
          {
#ifndef HAVE_SPACE_NAVIGATOR
            assert(!"Broken build, this code should be unreachable");
#else
            spnav_event* spnav_ev = static_cast<spnav_event*>(event.user.data1);

            switch(spnav_ev->type)
              {
                case SPNAV_EVENT_MOTION:
                  {
                    if (0)
                      std::cout << "MotionEvent: " 
                                << "("
                                << spnav_ev->motion.x << ", "
                                << spnav_ev->motion.y << ", "
                                << spnav_ev->motion.z
                                << ") ("
                                << spnav_ev->motion.rx << ", "
                                << spnav_ev->motion.ry << ", "
                                << spnav_ev->motion.rz
                                << std::endl;              

                    float factor = -abs(spnav_ev->motion.y)/10000.0f;

                    if (spnav_ev->motion.y > 0)
                      viewer->get_state().zoom(1.0f+factor);
                    else if (spnav_ev->motion.y < 0)
                      viewer->get_state().zoom(1.0f/(1.0f+factor));

                    viewer->get_state().move(Vector2f(-spnav_ev->motion.x / 10.0f,
                                                     +spnav_ev->motion.z / 10.0f));

                    if (spnav_allow_rotate)
                      viewer->get_state().rotate(spnav_ev->motion.ry / 200.0f);
                  }
                  break;
            
                case SPNAV_EVENT_BUTTON:
                  if (0)
                    std::cout << "ButtonEvent: " << spnav_ev->button.press << spnav_ev->button.bnum << std::endl;

                  if (spnav_ev->button.bnum == 0 && spnav_ev->button.press)
                    viewer->get_state().set_angle(0.0f);

                  if (spnav_ev->button.bnum == 1 && spnav_ev->button.press)
                    spnav_allow_rotate = !spnav_allow_rotate;
                  break;

                default:
                  assert(!"SpaceNavigator: Unhandled event");
              }

            delete spnav_ev;
#endif
          }
        else if (event.user.code == 1)
          {
            // New tile arrived
          }
        break;

      case SDL_QUIT:
        std::cout << "Viewer: SDL_QUIT received" << std::endl;
        quit = true;
        break;

      case SDL_VIDEOEXPOSE:
        break;

      case SDL_VIDEORESIZE:
        SDLFramebuffer::reshape(Size(event.resize.w, event.resize.h));
        break;

      case SDL_MOUSEMOTION:
        viewer->on_mouse_motion(Vector2i(event.motion.x,    event.motion.y),
                                Vector2i(event.motion.xrel, event.motion.yrel));
        break;

        // FIXME: SDL Reverses the mouse buttons when a grab is active!
      case SDL_MOUSEBUTTONDOWN:
        viewer->on_mouse_button_down(Vector2i(event.button.x, event.button.y),
                                     event.button.button);
        break;

      case SDL_MOUSEBUTTONUP:
        viewer->on_mouse_button_up(Vector2i(event.button.x, event.button.y),
                                   event.button.button);
        break;

      case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
          {
            case SDLK_ESCAPE:
              quit = true;
              break;

            default:
              viewer->on_key_down(event.key.keysym.sym);
              break;
          }
        break;


      case SDL_KEYUP:
        viewer->on_key_up(event.key.keysym.sym);
        break;                

      default:
        break;
    }
}

void
SDLViewer::run()
{
  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
      std::cout << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
      exit(1);
    }
  atexit(SDL_Quit); 

  Workspace workspace;

  SDLFramebuffer::set_video_mode(geometry, fullscreen, anti_aliasing);

  workspace.layout_aspect(4,3);

  viewer = std::auto_ptr<Viewer>(new Viewer(&workspace));

  Uint32 ticks = SDL_GetTicks();

#ifdef HAVE_SPACE_NAVIGATOR
  SpaceNavigator space_navigator;
  space_navigator.start();
#endif

  while(!quit)
    {     
      while (!file_queue.empty())
        {
          const FileEntry& entry = file_queue.front();
          workspace.add_image(entry);
          file_queue.pop();
        }

      if (viewer->is_active())
        {
          SDL_Event event;
          while (SDL_PollEvent(&event))
            {
              process_event(event);
            }

          Uint32 cticks = SDL_GetTicks();
          float delta = (cticks - ticks) / 1000.0f;
          ticks = cticks;

          viewer->update(delta);
          viewer->draw();
        }
      else
        {
          SDL_Event event;
          SDL_WaitEvent(NULL);
          while (SDL_PollEvent(&event))
            {
              process_event(event);
            }

          // FIXME: We should try to detect if we need a redraw and
          // only draw then, else we will redraw on each mouse motion
          viewer->draw();          
          ticks = SDL_GetTicks();
        }

      SDLFramebuffer::flip();

      // std::cout << "." << std::flush;

      SDL_Delay(30);
    }

#ifdef HAVE_SPACE_NAVIGATOR
  // How should be join stuff that is waiting for stuff?
  // space_navigator.join();
#endif

  std::cout << "SDLViewer: done" << std::endl;
}

/* EOF */
