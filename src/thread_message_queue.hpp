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

#ifndef HEADER_THREAD_MESSAGE_QUEUE_HPP
#define HEADER_THREAD_MESSAGE_QUEUE_HPP

#include <queue>
#include "thread.hpp"

template<class C>
class ThreadMessageQueue
{
private:
  Mutex mutex;
  std::queue<C> values;

public:
  ThreadMessageQueue()
  {
  }

  ~ThreadMessageQueue()
  {
  }

  void push(const C& value)
  {
    mutex.lock();
    values.push(value);
    mutex.unlock();
  }

  void pop()
  {
    mutex.lock();
    values.pop();
    mutex.unlock();
  }

  C front()
  {
    mutex.lock();
    C c(values.front());
    mutex.unlock();
    return c;
  }

  int size()
  {
    mutex.lock();
    int s = values.size();
    mutex.unlock();
    return s;
  }

  bool empty() 
  {
    mutex.lock();
    bool e = values.empty();
    mutex.unlock();
    return e;
  }

  void wait()
  {
    // FIXME: implement me properly
    SDL_Delay(100);
  }

  void wakeup()
  {
    // Implement me: Break the queue out of a wait()
  }

private:
  ThreadMessageQueue (const ThreadMessageQueue&);
  ThreadMessageQueue& operator= (const ThreadMessageQueue&);
};

#endif

/* EOF */
