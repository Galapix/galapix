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

#include <assert.h>
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
  return thread->run();
}

Thread::Thread()
  : thread(0)
{
}

Thread::~Thread()
{
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

void
Thread::start()
{
  assert(thread == 0);
  thread = SDL_CreateThread(launch_thread, this);
}

/* EOF */
