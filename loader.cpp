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
#include <iomanip>
#include <stdexcept>
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
Loader::request(Image* receiver)
{
  SDL_LockMutex(mutex);
  jobs.push_back(Job(receiver));
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

      int res = job.image->want_res;

      if (job.image->res != res && res != 0)
        {
          // Unlock
      
          std::string m = this->md5(job.image->url);
          std::ostringstream out;
          out << Filesystem::get_home() << "/.griv/cache/by_url/"
              << res << "/" << m.substr(0,2) << "/" << m.substr(2) << ".jpg";

          //std::cout << "Loading: " << job.filename << std::endl;
          //std::cout << out.str() << std::endl;
          SDL_Surface* img = IMG_Load(out.str().c_str());
          if (img)
            {
              job.image->receive(img);
            }
          else
            {
              // No thumbnail, assuming we need the original
              if (res > 1024)
                {
                  img = IMG_Load(job.image->url.substr(7).c_str()); // cut file:// part
                  job.image->receive(img);
                }
            }
        }
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

std::string
Loader::md5(const std::string& str)
{
  unsigned char hash[16]; /* enough size for MD5 */
  MHASH td = mhash_init(MHASH_MD5);
  if (td == MHASH_FAILED) 
    throw std::runtime_error("Failed to init MHash");
  
  mhash(td, str.c_str(), str.length());
  
  mhash_deinit(td, hash);

  std::ostringstream out;
  for (int i = 0; i < 16; i++) 
    out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);

  return out.str();
}

/* EOF */
