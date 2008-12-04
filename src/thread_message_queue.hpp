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
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

template<class C>
class ThreadMessageQueue
{
private:
  boost::mutex     mutex;
  boost::condition cond;
  std::queue<C>    values;

public:
  ThreadMessageQueue()
  {
  }

  ~ThreadMessageQueue()
  {
  }

  void push(const C& value)
  {
    {
      boost::mutex::scoped_lock lock(mutex);
      values.push(value);
    }
    cond.notify_one();
  }

  void pop()
  {
    boost::mutex::scoped_lock lock(mutex);
    values.pop();
  }

  C front()
  {
    boost::mutex::scoped_lock lock(mutex);
    C c(values.front());
    return c;
  }

  int size()
  {
    boost::mutex::scoped_lock lock(mutex);
    int s = values.size();
    return s;
  }

  bool empty() 
  {
    boost::mutex::scoped_lock lock(mutex);
    bool e = values.empty();
    return e;
  }

  void wait()
  {
    boost::mutex::scoped_lock lock(mutex);
    cond.wait(lock);
  }

  void wakeup()
  {
    boost::mutex::scoped_lock lock(mutex);
    cond.notify_one();   
  }

private:
  ThreadMessageQueue (const ThreadMessageQueue&);
  ThreadMessageQueue& operator= (const ThreadMessageQueue&);
};

#endif

/* EOF */
