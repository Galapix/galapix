/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#include "util/thread_pool.hpp"

#include <assert.h>
#include <logmich/log.hpp>

ThreadPool::ThreadPool(int num_threads, ShutdownPolicy shutdown_policy) :
  m_shutdown_policy(shutdown_policy),
  m_shutdown(false),
  m_forced_shutdown(false),
  m_threads(),
  m_queue()
{
  assert(num_threads > 0);

  for(int i = 0; i < num_threads; ++i)
  {
    m_threads.emplace_back([this]{ this->run();});
  }
}

ThreadPool::~ThreadPool()
{
  m_shutdown = true;
  if (m_shutdown_policy == kWaitForActiveTasks)
  {
    m_forced_shutdown = true;
  }

  m_queue.wakeup();

  for(auto& thread : m_threads)
  {
    thread.join();
  }
}

void
ThreadPool::schedule(const Task& task)
{
  m_queue.try_push(task);
}

void
ThreadPool::abort()
{
  m_forced_shutdown = true;
  m_shutdown = true;
}

void
ThreadPool::run()
{
  while(!(m_forced_shutdown || (m_shutdown && m_queue.empty())))
  {
    try
    {
      Task task;
      m_queue.wait_for_pop([this]{ return m_forced_shutdown || m_shutdown; });
      if (m_queue.try_pop(task))
      {
        task();
      }
    }
    catch(const std::exception& err)
    {
      log_error("exception while executing task: %1%", err.what());
    }
    catch(...)
    {
      log_error("unknown exception");
    }
  }
}

/* EOF */
