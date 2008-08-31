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
#include <assert.h>
#include <stdexcept>
#include "thread.hpp"

/*
  SDL_mutex* mutex;
  SDL_CreateMutex();
  SDL_DestroyMutex(mutex);
*/

Mutex::Mutex()
{
  mutex = SDL_CreateMutex();
}

Mutex::~Mutex()
{
  SDL_DestroyMutex(mutex);
}

void
Mutex::lock()
{
  SDL_LockMutex(mutex);
}

void
Mutex::unlock()
{
  SDL_UnlockMutex(mutex);
}

int launch_thread(void* thread_ptr)
{
  Thread* thread = static_cast<Thread*>(thread_ptr);
  try 
    {
      return thread->run();
    }
  catch(std::exception& err)
    {
      std::cout << "\nFatal Error: " << thread->get_name() << " (" << thread->get_id() << "): " << err.what() << std::endl;
      return EXIT_FAILURE;
    }
}

Thread::Thread(const std::string& name_)
  : thread(0),
    name(name_)
{
}

Thread::~Thread()
{
}

std::string
Thread::get_name() const
{
  return name;
}

int
Thread::join()
{
  int ret = 0;
  SDL_WaitThread(thread, &ret);
  return ret;
}

Uint32
Thread::get_id()
{
  return SDL_GetThreadID(thread);
}

Uint32
Thread::get_thread_id()
{
  return SDL_ThreadID();
}

void
Thread::start()
{
  assert(thread == 0);
  thread = SDL_CreateThread(launch_thread, this);
}

/* EOF */
