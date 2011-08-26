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

#include <assert.h>
#include <queue>
#include <condition_variable>
#include <mutex>

template<typename Data>
class ThreadMessageQueue2
{
private:
  std::queue<C>    m_values;
  int              m_max_size;

  std::mutex     m_mutex;
  std::condition_variable m_queue_not_empty_cond;
  std::condition_variable m_queue_not_full_cond;

public:
  ThreadMessageQueue2(int max_size = -1) :
    m_values(),
    m_max_size(max_size),
    m_mutex(),
    m_queue_not_empty_cond(),
    m_queue_not_full_cond()
  {}

  ~ThreadMessageQueue2()
  {
  }

  bool full() const
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_values.size() == m_max_size;
  }

  bool empty() const
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_values.empty();
  }


  bool try_push(const Data& data)
  {
    std::unique_lock<std::mutex> lock(the_mutex);

    if (m_values.size() == m_max_size)
    {
      return false;
    }
    else
    {
      // push the data to the queue
      m_queue.push(data);

      // notify that the queue is no longer empty
      lock.unlock();
      m_queue_not_empty_cond.notify_one();

      return true;
    }
  }

  void wait_and_push(const Data& data)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
   
    m_queue_not_full_cond.wait(lock, []{ return m_queue.size() != m_max_size; });

    // push the data to the queue    
    m_queue.push_back(data);

    // notify that the queue is no longer empty
    lock.unlock();
    m_queue_not_empty_cond.notify_one();
  }


  bool try_pop(Data& data_out)
  {
    std::unique_lock<std::mutex> lock(the_mutex);
    if(m_queue.empty())
    {
      return false;
    }
    else
    {        
      // pop the data
      data_out = m_queue.front();
      m_queue.pop();

      // notify that the queue is no longer full
      lock.unlock();
      m_queue_not_full_cond.notify_one();

      return true;
    }
  }

  void wait_and_pop(Data& data_out)
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_queue_not_empty_cond.wait(lock, []{ return !m_queue.empty(); });

    // pop the data
    data_out = m_queue.front();
    m_queue.pop();

    // notify that the queue is no longer full
    lock.unlock();
    m_queue_not_full_cond.notify_one();
  }

private:
  ThreadMessageQueue2 (const ThreadMessageQueue2&);
  ThreadMessageQueue2& operator= (const ThreadMessageQueue2&);
};

#endif

/* EOF */
