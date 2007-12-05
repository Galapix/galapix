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
#include "command_line.hpp"
#include "file_entry_cache.hpp"

float x_offset = 0.0f;
float y_offset = 0.0f;
std::string config_home;
bool force_redraw = true;
bool highquality  = false;
FileEntryCache* cache;

Griv::Griv()
{
  zoom_in_pressed = false;
  zoom_out_pressed = false;
  grid_size = 2;
  draw_grid = false;
  grid_color = true;
  drag_toggle = false;
  gamma = 1.0f;
}

Griv::~Griv()
{
}

void
Griv::gl_draw_grid(int grid_size)
{
  glBindTexture(GL_TEXTURE_2D, 0);
  glBegin(GL_LINES);
  if (grid_color)
    glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
  else
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);

  for(int x = Framebuffer::get_width()/grid_size;
      x < Framebuffer::get_width(); 
      x += Framebuffer::get_width()/grid_size)
    {
      glVertex2f(x, 0);
      glVertex2f(x, Framebuffer::get_height());
    }

  for(int y = Framebuffer::get_height()/grid_size;
      y < Framebuffer::get_height(); y += Framebuffer::get_height()/grid_size)
    {
      glVertex2f(0, y);
      glVertex2f(Framebuffer::get_width(), y);
    }
  glEnd();
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
            else if (event.key.keysym.sym == SDLK_F10)
              {
                cache->print();
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
            else if (event.key.keysym.sym == SDLK_s)
              {
                grid_size *= 2;
                if (grid_size > 128)
                  grid_size = 2;
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_g)
              {
                draw_grid = !draw_grid;
                std::cout << "Drawing grid: " << draw_grid << " " << grid_size << std::endl;
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_t)
              {
                grid_color = !grid_color;
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_p)
              {
                std::cout << "---------------------------------------------------------" << std::endl;
                for(int i = 0; i < int(workspace->images.size()); ++i)
                  {
                    if (workspace->images[i]->is_visible())
                      {
                        std::cout << workspace->images[i]->url << "\t"
                                  << workspace->images[i]->original_width << "x"
                                  << workspace->images[i]->original_height
                                  << std::endl;
                      }
                  }
                std::cout << "---------------------------------------------------------" << std::endl;
              }
            else if (event.key.keysym.sym == SDLK_h)
              {
                highquality = !highquality;
                force_redraw = true;
                std::cout << "Highquality: " << highquality << std::endl;
              }
            else if (event.key.keysym.sym == SDLK_PAGEUP)
              {
                gamma *= 1.1f;
                SDL_SetGamma(gamma, gamma, gamma);
              }
            else if (event.key.keysym.sym == SDLK_PAGEDOWN)
              {
                gamma /= 1.1f;
                SDL_SetGamma(gamma, gamma, gamma);
              }
            else if (event.key.keysym.sym == SDLK_END)
              {
                gamma = 1.0f;
                SDL_SetGamma(gamma, gamma, gamma);
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
            else if (event.key.keysym.sym == SDLK_7)
              {
                workspace->set_zoom(1024.0f);
              }
            else if (event.key.keysym.sym == SDLK_8)
              {
                workspace->set_zoom(2048.0f);
              }
            else if (event.key.keysym.sym == SDLK_9)
              {
                workspace->layout_sort(true);
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_0)
              {
                workspace->layout_sort(false);
                force_redraw = true;
              }
            else if (event.key.keysym.sym == SDLK_r)
              {
                workspace->layout_random();
                force_redraw = true;
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
            else if (event.key.keysym.sym == SDLK_d)
              {
                drag_toggle = !drag_toggle;
                if (!drag_toggle)
                  {
                    SDL_ShowCursor(SDL_ENABLE);
                    SDL_WM_GrabInput(SDL_GRAB_OFF);
                  }
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
            if (0)
            std::cout << event.motion.xrel << " " << event.motion.yrel  << " "
                      << x_offset << " " << y_offset 
                      << std::endl;
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
                if (!drag_toggle)
                  {
                    drag_n_drop = event.button.state;
                  }
                else if (event.button.state == SDL_PRESSED)
                  {
                    drag_n_drop = !drag_n_drop;
                    if (drag_n_drop)
                      {
                        SDL_ShowCursor(SDL_DISABLE);
                        SDL_WM_GrabInput(SDL_GRAB_ON);
                      }
                    else
                      {
                        SDL_ShowCursor(SDL_ENABLE);
                        SDL_WM_GrabInput(SDL_GRAB_OFF);
                      }
                  }
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
      if (drag_toggle && drag_n_drop)
        workspace->zoom_out(0,
                            0,
                            1.0f + zoom_speed * delta);
      else
        workspace->zoom_out(mouse_x - Framebuffer::get_width()/2,
                            mouse_y - Framebuffer::get_height()/2,
                            1.0f + zoom_speed * delta);
      loader.clear();
    }
  else if (!zoom_out_pressed && zoom_in_pressed)
    {
      if (drag_toggle && drag_n_drop)
        workspace->zoom_in(0,
                            0,
                            1.0f + zoom_speed * delta);
      else
        workspace->zoom_in(mouse_x - Framebuffer::get_width()/2,
                           mouse_y - Framebuffer::get_height()/2,
                           1.0f + zoom_speed * delta);    
      loader.clear();
    }
}



int
Griv::main(int argc, char** argv)
{
  try {
    std::cout << "Processing command line arguments... " << std::flush;
    CommandLine argp;
      
    argp.add_usage("[OPTION]... [FILE]...");
    argp.add_doc("Griv - A ZUI image viewer\n");
    argp.add_option('f', "file", "FILE", "Load URL list from FILE");
    argp.add_option('h', "help", "", "Print this help");

    try {
      argp.parse_args(argc, argv);
    } catch(std::exception& err) {
      std::cout << "Error: CommandLine: " << err.what() << std::endl;
      exit(EXIT_FAILURE);
    }

    std::vector<std::string> url_list;
    std::vector<std::string> pathnames;
    while(argp.next())
      {
        switch(argp.get_key())
          {
            case 'h':
              argp.print_help();
              exit(EXIT_SUCCESS);
              break;

            case 'f':
              Filesystem::readlines_from_file(argp.get_argument(), url_list);
              break;

            case CommandLine::REST_ARG:
              pathnames.push_back(argp.get_argument());
              break;

            default:
              std::cout << "Unhandled argument: " << argp.get_key() << std::endl;
              exit(EXIT_FAILURE);
              break;
          }
      };
    std::cout << "done" << std::endl;    

    std::cout << "Init filesystem... " << std::flush;
    Filesystem::init();
    std::cout << "done" << std::endl;

    std::cout << "Loading cache... " << std::flush;
    cache = new FileEntryCache(Filesystem::get_home() + "/.griv/cache/file.cache");
    std::cout << "done" << std::endl;

    std::cout << "Generating url list... " << std::flush;
    for(std::vector<std::string>::iterator i = pathnames.begin(); i != pathnames.end(); ++i)
      Filesystem::generate_jpeg_file_list(*i, url_list);
    std::cout << "done" << std::endl;

    Framebuffer::init();
  
    workspace = new Workspace();

    for(std::vector<std::string>::iterator i = url_list.begin(); i != url_list.end(); ++i)
      {
        workspace->add(*i);
        if ((i - url_list.begin() + 1) % 29 == 0)
          std::cout << "Adding images to workspace... " 
                    << (i - url_list.begin() + 1) << "/" << url_list.size() 
                    << "\r" << std::flush;
      }
    std::cout << "done" << std::endl;
  
    std::cout << "Saving cache" << std::endl;
    cache->save(Filesystem::get_home() + "/.griv/cache/file.cache");

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
    old_rotation = 0;
    next_redraw = 0;

    loader.start_thread();

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
                old_rotation != workspace->rotation ||
                (force_redraw && (next_redraw < SDL_GetTicks())) ||
                workspace->reorganize)
              {
                workspace->update_resources();

                force_redraw = false;

                Framebuffer::clear();
                workspace->update(delta / 1000.0f);
                workspace->draw();
              
                if (draw_grid)
                  gl_draw_grid(grid_size);

                Framebuffer::flip();

                old_res = workspace->res;
                old_x_offset = x_offset;
                old_y_offset = y_offset;
                old_rotation = workspace->rotation;
                next_redraw = SDL_GetTicks() + 1000;
              }
            else
              {
                if (workspace->update_resources())
                  force_redraw = true;
                else
                  SDL_Delay(30); // nothing to do, so sleep
              }
          }
      }

    loader.stop_thread();

    delete workspace;

    Framebuffer::deinit();
    Filesystem::deinit();
  } catch(std::exception& err) {
    std::cout << "ERROR: " << err.what() << std::endl;
  }

  return 0;
}

int main(int argc, char** argv)
{
  try {
    Griv griv;
    return griv.main(argc, argv);
  } catch(std::exception& err) {
    std::cout << "Griv: uncatched exception: " << err.what() << std::endl;
    return EXIT_FAILURE;
  }
}

/* EOF */
