#include <vector>
#include <math.h>
#include <sstream>
#include <sys/types.h>
#include <stdexcept>
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"

#include "workspace.hpp"
#include "loader.hpp"
#include "image.hpp"

SDL_Surface* screen = 0;
int x_offset = 0;
int y_offset = 0;
std::string config_home;
bool force_redraw = true;

int main(int argc, char** argv)
{
  char* home;
  if ((home = getenv("HOME")))
    {
      config_home = home;
    }
  else
    {
      std::cout << "Couldn't get HOME environment variable" << std::endl;
      return 0;
    }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
    std::cout << "Unable to initialize SDL: " << SDL_GetError() << std::endl;
    exit(1);
  }
  atexit(SDL_Quit); 
  
  Uint32 flags = 0;
  
  if (0)
    {
      //flags |= SDL_FULLSCREEN;
      screen = SDL_SetVideoMode(1152, 864, 0, flags);
    }
  else
    {
      flags |= SDL_RESIZABLE;
      screen = SDL_SetVideoMode(800, 600, 0, flags);
    }
    
  if (screen == NULL) 
    {
      std::cout << "Unable to set video mode: " << SDL_GetError() << std::endl;
      exit(1);
    }

  SDL_WM_SetCaption("Griv 0.0.1", 0 /* icon */);

  SDL_EnableUNICODE(1);

  Image::init();

  Workspace workspace;

  for(int i = 1; i < argc; ++i)
    {
      workspace.add(argv[i]);
    }

  bool drag_n_drop = false;
  int old_res = -1;
  int old_x_offset = -1;
  int old_y_offset = -1;
  Uint32 next_redraw = 0;
  loader.launch_thread();
  while(true)
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
        {
          switch(event.type)
            {
              case SDL_QUIT: // FIXME: make this into a GameEvent
                exit(1);
                break;

              case SDL_VIDEORESIZE:
                screen = SDL_SetVideoMode(event.resize.w, event.resize.h, 0, SDL_RESIZABLE);
                force_redraw = true;
                break;

              case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                  {
                    exit(1);
                  }
                else if (event.key.keysym.sym == SDLK_RETURN)
                  {
                    loader.process_job();
                  }
                else if (event.key.keysym.sym == SDLK_F11)
                  {
                    if (flags & SDL_FULLSCREEN)
                      flags = 0;
                    else
                      flags |= SDL_FULLSCREEN;
                    
                    screen = SDL_SetVideoMode(1152, 864, 0, flags);
                    force_redraw = true;
                  }
                else if (event.key.keysym.sym == SDLK_SPACE)
                  {
                    x_offset = 0;
                    y_offset = 0;
                  }
                else if (event.key.keysym.sym == SDLK_b)
                  {
                    std::cout << x_offset << ", " << y_offset << std::endl;
                  }
                break;


              case SDL_MOUSEMOTION:
                if (drag_n_drop)
                  {
                    x_offset += event.motion.xrel*4;
                    y_offset += event.motion.yrel*4;
                  }
                break;

              case SDL_MOUSEBUTTONDOWN:
              case SDL_MOUSEBUTTONUP:
                
                if (event.button.button == 3 ||
                    event.button.button == 5)
                  {
                    if (event.button.state == SDL_PRESSED)
                      {
                        //std::cout << "zoom out" << std::endl;
                        workspace.zoom_out();
                        loader.clear();
                      }
                  }
                else if (event.button.button == 1 ||
                         event.button.button == 4)
                  {
                    if (event.button.state == SDL_PRESSED)
                      {
                        //std::cout << "zoom in" << std::endl;
                        workspace.zoom_in();
                        loader.clear();
                      }
                  }
                else if (event.button.button == 2)
                  {
                    drag_n_drop = event.button.state;
                  }
                break;
            }
        }

      if (workspace.res != old_res ||
          old_x_offset != x_offset ||
          old_y_offset != y_offset ||
          (force_redraw && (next_redraw < SDL_GetTicks() || loader.empty())))
        {
          force_redraw = false;
          SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
          workspace.draw();
          SDL_Flip(screen);

          old_res = workspace.res;
          old_x_offset = x_offset;
          old_y_offset = y_offset;
          next_redraw = SDL_GetTicks() + 500;
        }
      else
        {
          SDL_Delay(5);
        }
    }

  return 0;
}

/* EOF */
