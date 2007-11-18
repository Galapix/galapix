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
bool highquality  = true;

Griv::Griv()
{
  zoom_in_pressed = false;
  zoom_out_pressed = false;
}

Griv::~Griv()
{
}

void
Griv::process_events(float delta)
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
            else if (event.key.keysym.sym == SDLK_h)
              {
                highquality = !highquality;
                force_redraw = true;
                std::cout << "Highquality: " << highquality << std::endl;
              }
            else if (event.key.keysym.sym == SDLK_9)
              {
                workspace->layout(4,3);
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_8)
              {
                workspace->layout(8,3);
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_7)
              {
                workspace->layout(16,3);
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_1)
              {
                workspace->set_zoom(16.0f);
              }
            else if (event.key.keysym.sym == SDLK_2)
              {
                workspace->set_zoom(32.0f);
              }
            else if (event.key.keysym.sym == SDLK_3)
              {
                workspace->set_zoom(64.0f);
              }
            else if (event.key.keysym.sym == SDLK_4)
              {
                workspace->set_zoom(128.0f);
              }
            else if (event.key.keysym.sym == SDLK_5)
              {
                workspace->set_zoom(256.0f);
              }
            else if (event.key.keysym.sym == SDLK_6)
              {
                workspace->set_zoom(512.0f);
              }
            else if (event.key.keysym.sym == SDLK_UP)
              {
                workspace->rotation = 0.0f;
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_DOWN)
              {
                workspace->rotation += 90.0f;
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_b)
              {
                std::cout << x_offset << ", " << y_offset << std::endl;
              }
            break;


          case SDL_MOUSEMOTION:
            mouse_x = event.motion.x;
            mouse_y = event.motion.y;
            if (drag_n_drop)
              {
                x_offset += event.motion.xrel*4;
                y_offset += event.motion.yrel*4;
              }
            break;

          case SDL_MOUSEBUTTONDOWN:
          case SDL_MOUSEBUTTONUP:
                
            if (event.button.button == 5)
              {
                if (event.button.state == SDL_PRESSED)
                  {
                    workspace->zoom_out(event.button.x - Framebuffer::get_width()/2,
                                        event.button.y - Framebuffer::get_height()/2,
                                        1.1f);
                    loader.clear();
                  }
              }
            else if (event.button.button == 4)
              {
                if (event.button.state == SDL_PRESSED)
                  {
                    workspace->zoom_in(event.button.x - Framebuffer::get_width()/2,
                                       event.button.y - Framebuffer::get_height()/2,
                                       1.1f);
                    loader.clear();
                  }
              }
            else if (event.button.button == 1)
              {
                zoom_in_pressed = (event.button.state == SDL_PRESSED);
              }
            else if (event.button.button == 3)
              {
                zoom_out_pressed = (event.button.state == SDL_PRESSED);
              }
            else if (event.button.button == 2)
              {
                drag_n_drop = event.button.state;
              }
            break;
        }
    } 
  
  int numkeys;
  Uint8* keys = SDL_GetKeyState(&numkeys);

  if (keys[SDLK_LEFT])
    {
      workspace->rotation -= 200.0f * delta;
      force_redraw = true;
    }
  
  if (keys[SDLK_RIGHT])
    {
      workspace->rotation += 200.0f * delta;
      force_redraw = true;
    }

  float zoom_speed = 3.0f;

  if (zoom_out_pressed && !zoom_in_pressed)
    {
      workspace->zoom_out(mouse_x - Framebuffer::get_width()/2,
                          mouse_y - Framebuffer::get_height()/2,
                          1.0f + zoom_speed * delta);
      loader.clear();
    }
  else if (!zoom_out_pressed && zoom_in_pressed)
    {
      workspace->zoom_in(mouse_x - Framebuffer::get_width()/2,
                         mouse_y - Framebuffer::get_height()/2,
                         1.0f + zoom_speed * delta);    
      loader.clear();
    }
}

int
Griv::main(int argc, char** argv)
{
  Filesystem::init();
  Framebuffer::init();
  
  workspace = new Workspace();

  for(int i = 1; i < argc; ++i)
    {
      workspace->add(argv[i]);
    }
  workspace->layout(4,3);
  std::cout << " done" << std::endl;

  std::cout << workspace->size() << " images scanned" << std::endl;

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

  Uint32 ticks = SDL_GetTicks();
  while(true)
    {
      Uint32 cticks = SDL_GetTicks();
      int delta = cticks - ticks;
      if (delta > 0)
        {
          ticks = cticks;
          process_events(delta / 1000.0f);

          if (workspace->res != old_res ||
              old_x_offset != x_offset ||
              old_y_offset != y_offset ||
              (force_redraw && (next_redraw < SDL_GetTicks() || loader.empty())) ||
              workspace->reorganize)
            {
              force_redraw = false;

              Framebuffer::clear();
              workspace->update(delta / 1000.0f);
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
    }

  delete workspace;

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
