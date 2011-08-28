/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_JOB_THREAD_MESSAGE_QUEUE2_HPP
#define HEADER_GALAPIX_JOB_THREAD_MESSAGE_QUEUE2_HPP

#include <assert.h>
#include <queue>
#include <condition_variable>
#include <mutex>

template<typename Data>
class ThreadMessageQueue2
{
private:
  std::queue<Data> m_queue;
  int              m_max_size;

  mutable std::mutex     m_mutex;
  std::condition_variable m_queue_not_empty_cond;
  std::condition_variable m_queue_not_full_cond;

public:
  ThreadMessageQueue2(int max_size = -1) :
    m_queue(),
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
    return m_queue.size() == m_max_size;
  }

  bool empty() const
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_queue.empty();
  }

  /** Try to push data on the queue, if the queue is full, fail and return false */
  bool try_push(const Data& data)
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (static_cast<int>(m_queue.size()) == m_max_size)
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

  /** Push data on the queue, if the queue is currently full, wait
      till it is no longer full */
  void wait_and_push(const Data& data)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
   
    m_queue_not_full_cond.wait(lock, [this]{ return static_cast<int>(m_queue.size()) != m_max_size; });

    // push the data to the queue    
    m_queue.push(data);

    // notify that the queue is no longer empty
    lock.unlock();
    m_queue_not_empty_cond.notify_one();
  }

  /** Waits till the queue is ready to accept a push */
  void wait_for_push()
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue_not_full_cond.wait(lock, [this]{ return static_cast<int>(m_queue.size()) != m_max_size; });
  }

  /** Try pop data from the queue, if it's empty return false */
  bool try_pop(Data& data_out)
  {
    std::unique_lock<std::mutex> lock(m_mutex);
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

  /** Pop data from the queue, if it's empty, wait till data is available */
  void wait_and_pop(Data& data_out)
  {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_queue_not_empty_cond.wait(lock, [this]{ return !m_queue.empty(); });

    // pop the data
    data_out = m_queue.front();
    m_queue.pop();

    // notify that the queue is no longer full
    lock.unlock();
    m_queue_not_full_cond.notify_one();
  }

  /** wait till the queue allows a pop */
  void wait_for_pop()
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue_not_empty_cond.wait(lock, [this]{ return !m_queue.empty(); });
  }

private:
  ThreadMessageQueue2 (const ThreadMessageQueue2&);
  ThreadMessageQueue2& operator= (const ThreadMessageQueue2&);
};

#endif

/* EOF */
