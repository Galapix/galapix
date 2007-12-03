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

#ifndef HEADER_LOADER_HPP
#define HEADER_LOADER_HPP

#include "SDL.h"
#include "SDL_thread.h"

#include <string>
#include <vector>

class ThumbnailManager;
class Image;

struct Job
{
  Image* image;

  Job(Image* image)
    : image(image)
  {}
};

/** */
class Loader
{
private:
  typedef std::vector<Job> Jobs;
  Jobs jobs;
  ThumbnailManager* store;
  SDL_mutex* mutex;
  SDL_Thread* thread;
  bool keep_thread_running;

public:
  static int thread_func(void*);

  Loader();
  ~Loader();

  void start_thread();
  void stop_thread();

  void request(Image* receiver);
  void process_job();
  void clear();
  bool empty();

private:
  Loader (const Loader&);
  Loader& operator= (const Loader&);
};

extern Loader loader;

#endif

/* EOF */
