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

#ifndef HEADER_GALAPIX_JOB_THREAD_MESSAGE_QUEUE_HPP
#define HEADER_GALAPIX_JOB_THREAD_MESSAGE_QUEUE_HPP

#include <queue>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

template<class C>
class ThreadMessageQueue
{
private:
  boost::mutex     m_mutex;
  boost::condition m_wait_cond;
  boost::condition m_full_cond;
  std::queue<C>    m_values;
  int              m_max_size;

public:
  ThreadMessageQueue(int max_size = -1) :
    m_mutex(),
    m_wait_cond(),
    m_full_cond(),
    m_values(),
    m_max_size(max_size)
  {}

  ~ThreadMessageQueue()
  {
  }

  void push(const C& value)
  {
    boost::mutex::scoped_lock lock(m_mutex);
    if(m_max_size != -1 && static_cast<int>(m_values.size()) == m_max_size)
    {
      while(static_cast<int>(m_values.size()) == m_max_size)
      {
        m_full_cond.wait(lock);
      }
      assert(static_cast<int>(m_values.size()) < m_max_size);
    }
    m_values.push(value);
    m_wait_cond.notify_all();
  }

  void pop()
  {
    boost::mutex::scoped_lock lock(m_mutex);
    m_values.pop();
    m_full_cond.notify_all();
  }

  C front()
  {
    boost::mutex::scoped_lock lock(m_mutex);
    assert(!m_values.empty());
    C c(m_values.front());
    return c;
  }

  int size()
  {
    boost::mutex::scoped_lock lock(m_mutex);
    int s = m_values.size();
    return s;
  }

  bool empty() 
  {
    boost::mutex::scoped_lock lock(m_mutex);
    bool e = m_values.empty();
    return e;
  }

  void wait()
  {
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_values.empty())
    {
      m_wait_cond.wait(lock);
    }
  }

  void wakeup()
  {
    boost::mutex::scoped_lock lock(m_mutex);
    m_wait_cond.notify_all();
  }

private:
  ThreadMessageQueue (const ThreadMessageQueue&);
  ThreadMessageQueue& operator= (const ThreadMessageQueue&);
};

#endif

/* EOF */
