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

#ifndef HEADER_THREAD_HPP
#define HEADER_THREAD_HPP

#include <string>
#include "SDL.h"

class Mutex 
{
private:
  SDL_mutex* mutex;

public:
  Mutex();
  ~Mutex();

  void lock();
  void unlock();

private:
  Mutex (const Mutex&);
  Mutex& operator= (const Mutex&);
};

class Thread
{
private:
  SDL_Thread* thread;
  std::string name;

public:
  Thread(const std::string& name);
  virtual ~Thread();

  void start();
  int  join();
  Uint32 get_id();
  std::string get_name() const; 
  
  virtual int run() =0;

  static Uint32 get_thread_id();

private:
  Thread (const Thread&);
  Thread& operator= (const Thread&);
};

#endif

/* EOF */
