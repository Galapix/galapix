#include <vector>
#include <math.h>
#include <sstream>
#include <sys/types.h>
#include <stdexcept>
#include <iostream>
#include "SDL.h"
#include "SDL_image.h"

#include "display.hpp"
#include "workspace.hpp"
#include "loader.hpp"
#include "image.hpp"

float x_offset = 0.0f;
float y_offset = 0.0f;
std::string config_home;
bool force_redraw = true;

Griv::Griv()
{
}

Griv::~Griv()
{
}

void
Griv::process_events()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
    {
      switch(event.type)
        {
          case SDL_QUIT: // FIXME: make this into a GameEvent
            exit(1);
            break;

          case SDL_VIDEOEXPOSE: // FIXME: make this into a GameEvent
            force_redraw = true;
            break;

          case SDL_VIDEORESIZE:
            Framebuffer::resize(event.resize.w, event.resize.h);
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
                Framebuffer::toggle_fullscreen();
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
                    workspace->zoom_out(event.button.x - Framebuffer::get_width()/2,
                                        event.button.y - Framebuffer::get_height()/2,
                                        1.1f);
                    loader.clear();
                  }
              }
            else if (event.button.button == 1 ||
                     event.button.button == 4)
              {
                if (event.button.state == SDL_PRESSED)
                  {
                    //std::cout << "zoom in" << std::endl;
                    workspace->zoom_in(event.button.x - Framebuffer::get_width()/2,
                                       event.button.y - Framebuffer::get_height()/2,
                                       1.1f);
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
}

int
Griv::main(int argc, char** argv)
{
  Filesystem::init();
  Framebuffer::init();
  Image::init();

  workspace = new Workspace();

  for(int i = 1; i < argc; ++i)
    {
      workspace->add(argv[i]);
    }
  std::cout << " done" << std::endl;

  {
    int w = int(sqrt(4 * workspace->size() / 3));
    x_offset = (-w/2) * 4;
    y_offset = (-(w*3/4)/2) * 4;
  }
  
  drag_n_drop = false;
  old_res = -1;
  old_x_offset = -1;
  old_y_offset = -1;
  next_redraw = 0;

  loader.launch_thread();

  while(true)
    {
      process_events();

      if (workspace->res != old_res ||
          old_x_offset != x_offset ||
          old_y_offset != y_offset ||
          (force_redraw && (next_redraw < SDL_GetTicks() || loader.empty())))
        {
          force_redraw = false;

          Framebuffer::clear();
          workspace->draw();
          Framebuffer::flip();

          old_res = workspace->res;
          old_x_offset = x_offset;
          old_y_offset = y_offset;
          next_redraw = SDL_GetTicks() + 1000;
        }
      else
        {
          SDL_Delay(5);
        }
    }

  delete workspace;

  Image::deinit();
  Framebuffer::deinit();
  Filesystem::deinit();

  return 0;
}

int main(int argc, char** argv)
{
  Griv griv;
  return griv.main(argc, argv);
}

/* EOF */
