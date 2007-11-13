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

#include <iostream>
#include "filesystem.hpp"
#include "SDL_image.h"
#include "image.hpp"
#include "loader.hpp"

Loader loader;

Loader::Loader()
{
  mutex  = SDL_CreateMutex();
  thread = 0;
}

Loader::~Loader()
{
  SDL_DestroyMutex(mutex);
}

int
Loader::thread_func(void*)
{
  while(1)
    {
      loader.process_job();

      if (loader.empty())
        SDL_Delay(100);
    }
  return 0;
}

void
Loader::launch_thread()
{
  thread = SDL_CreateThread(&Loader::thread_func, 0);
}

void
Loader::request(const std::string& uid, int res, Image* receiver)
{
  std::ostringstream out;
  out << Filesystem::get_home() << "/.griv/" << res << "/" << uid << ".jpg";

  SDL_LockMutex(mutex);
  jobs.push_back(Job(out.str(), receiver));
  SDL_UnlockMutex(mutex);
}

void
Loader::process_job()
{ // Thread this
  SDL_LockMutex(mutex);
  if (!jobs.empty())
    {
      // Lock
      Job job = jobs.back();
      jobs.pop_back();
      // Unlock

      //std::cout << "Loading: " << job.filename << std::endl;
      SDL_Surface* img = IMG_Load(job.filename.c_str());
      job.image->receive(img);
    }
  SDL_UnlockMutex(mutex);
}

void
Loader::clear()
{
  SDL_LockMutex(mutex);
  for(Jobs::iterator i = jobs.begin(); i != jobs.end(); ++i)
      (*i).image->receive(0);

  jobs.clear();
  SDL_UnlockMutex(mutex);
}

bool
Loader::empty()
{
  return jobs.empty();
}

/* EOF */
