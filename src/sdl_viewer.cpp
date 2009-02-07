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

#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include "math/rgba.hpp"
#include "filesystem.hpp"
#include "file_entry.hpp"
#include "workspace.hpp"
#include "sdl_framebuffer.hpp"
#include "viewer.hpp"
#include "framebuffer.hpp"
#include "plugins/png.hpp"
#include "sdl_viewer.hpp"
#include "viewer_state.hpp"
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
SDLViewer::process_event(const SDL_Event& event)
{
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
        switch(event.button.button)
          {
            case SDL_BUTTON_WHEELUP:
              viewer->get_state().zoom(1.1f, Vector2i(event.button.x, event.button.y));
              break;
              
            case SDL_BUTTON_WHEELDOWN:
              viewer->get_state().zoom(1.0f/1.1f, Vector2i(event.button.x, event.button.y));
              break;

            default:
              viewer->on_mouse_button_down(Vector2i(event.button.x, event.button.y),
                                           event.button.button);
              break;
          }
        break;

      case SDL_MOUSEBUTTONUP:
        switch(event.button.button)
          {
            default:
              viewer->on_mouse_button_up(Vector2i(event.button.x, event.button.y),
                                         event.button.button);
              break;
          }
        break;

      case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
          {
            case SDLK_ESCAPE:
              quit = true;
              break;
              
            case SDLK_d:
              viewer->zoom_to_selection();
              break;

            case SDLK_KP_PLUS:
              viewer->get_state().zoom(1.25f);
              break;

            case SDLK_KP_MINUS:
              viewer->get_state().zoom(1.0f/1.25f);
              break;

            case SDLK_KP8:
              viewer->get_state().set_offset(viewer->get_state().get_offset() + Vector2f(0.0f, +128.0f));
              break;

            case SDLK_KP2:
              viewer->get_state().set_offset(viewer->get_state().get_offset() + Vector2f(0.0f, -128.0f));
              break;

            case SDLK_KP4:
              viewer->get_state().set_offset(viewer->get_state().get_offset() + Vector2f(+128.0f, 0.0f));
              break;

            case SDLK_KP6:
              viewer->get_state().set_offset(viewer->get_state().get_offset() + Vector2f(-128.0f, 0.0f));
              break;

            case SDLK_p:
              viewer->set_pan_tool();
              break;

            case SDLK_r:
              viewer->set_move_rotate_tool();
              break;

            case SDLK_k:
              viewer->cleanup_cache();
              break;
        
            case SDLK_z:
              viewer->set_zoom_tool();
              break;

            case SDLK_m:
              viewer->set_move_resize_tool();
              break;

            case SDLK_h:
              viewer->zoom_home();
              break;

            case SDLK_s:
              viewer->sort_image_list();
              break;

            case SDLK_n:
              viewer->shuffle_image_list();
              break;

            case SDLK_F12:
              {
                SoftwareSurface surface = Framebuffer::screenshot();
                // FIXME: Could do this in a worker thread to avoid pause on screenshotting
                for(int i = 0; ; ++i)
                  {
                    std::string outfile = (boost::format("/tmp/galapix-screenshot-%04d.png") % i).str();
                    if (!Filesystem::exist(outfile.c_str()))
                      {
                        PNG::save(surface, outfile);
                        std::cout << "Screenshot written to " << outfile << std::endl;
                        break;
                      }
                  }
              }
              break;              

            case SDLK_i:
              viewer->isolate_selection();
              break;

            case SDLK_DELETE:
              viewer->delete_selection();
              break;

            case SDLK_y:
              viewer->set_grid_tool();
              break;

            case SDLK_F6:
              viewer->increase_brightness();
              break;

            case SDLK_F7:
              viewer->decrease_brightness();
              break;        

            case SDLK_F8:
              viewer->increase_contrast();
              break;        

            case SDLK_F9:
              viewer->decrease_contrast();
              break;        

            case SDLK_F10:
              viewer->reset_gamma();
              break;

            case SDLK_PAGEUP:
              viewer->increase_gamma();
              break;

            case SDLK_PAGEDOWN:
              viewer->decrease_gamma();
              break;

            case SDLK_1:
              viewer->layout_auto();
              break;

            case SDLK_2:
              viewer->layout_tight();
              break;

            case SDLK_3:
              viewer->layout_random();
              break;

            case SDLK_4:
              viewer->layout_solve_overlaps();
              break;

            case SDLK_6:
              viewer->layout_vertical();
              break;

            case SDLK_g:
              viewer->toggle_grid();
              break;

            case SDLK_f:
              viewer->toggle_pinned_grid();
              break;

            case SDLK_F11:
              SDLFramebuffer::toggle_fullscreen();
              break;

            case SDLK_F2:
              viewer->load();
              break;

            case SDLK_F3:
              viewer->save();
              break;

            case SDLK_j:
              viewer->clear_quad_tree();
              break;
                
            case SDLK_c:
              viewer->clear_cache();
              break;

            case SDLK_F5:
              viewer->refresh_selection();
              break;
              
            case SDLK_q:
              viewer->build_quad_tree();
              break;

            case SDLK_t:
              viewer->toggle_trackball_mode();
              break;

            case SDLK_UP:
            case SDLK_DOWN:
              viewer->reset_view_rotation();
              break;

            case SDLK_LEFT:
              viewer->rotate_view_270();
              break;

            case SDLK_RIGHT:
              viewer->rotate_view_90();
              break;

            case SDLK_b:
              viewer->toggle_background_color();
              break;

            case SDLK_SPACE:
              viewer->print_images();
              break;
              
            case SDLK_l:
              viewer->print_state();
              break;

            case SDLK_0:
              viewer->print_info();
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
  assert(workspace);

  if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
      std::cout << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
      exit(1);
    }
  atexit(SDL_Quit); 

  SDLFramebuffer::set_video_mode(geometry, fullscreen, anti_aliasing);

  workspace->layout_aspect(4,3);

  viewer = std::auto_ptr<Viewer>(new Viewer(workspace));

  Uint32 ticks = SDL_GetTicks();

#ifdef HAVE_SPACE_NAVIGATOR
  SpaceNavigator space_navigator;
  boost::thread  space_navigator_thread(boost::bind(&SpaceNavigator::run, &space_navigator));
#endif

  while(!quit)
    {     
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

      // FIXME: Higher values mean less CPU use for drawing and more
      // for loading, which is nice for low-end CPUs, but not so nice
      // for high-end CPUs, maybe make this configurable
      SDL_Delay(10);
    }

#ifdef HAVE_SPACE_NAVIGATOR
  // How should be join stuff that is waiting for stuff?
  // space_navigator_thread.join();
#endif

  std::cout << "SDLViewer: done" << std::endl;
}

/* EOF */
