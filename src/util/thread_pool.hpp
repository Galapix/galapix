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

#ifndef HEADER_GALAPIX_SCHEDULER_THREAD_POOL_HPP
#define HEADER_GALAPIX_SCHEDULER_THREAD_POOL_HPP

#include <functional>
#include <thread>
#include <vector>

#include "job/thread_message_queue2.hpp"

class ThreadPool
{
public:
  typedef std::function<void ()> Task;

  enum ShutdownPolicy {
    kWaitForAllTasks,
    kWaitForActiveTasks
  };

private:
  ShutdownPolicy m_shutdown_policy;
  bool m_shutdown;
  bool m_forced_shutdown;
  std::vector<std::thread> m_threads;

  ThreadMessageQueue2<Task> m_queue;

public:
  ThreadPool(int num_threads = 4, ShutdownPolicy shutdown_policy = kWaitForAllTasks);
  ~ThreadPool();

  void schedule(const Task& task);
  void abort();

  bool is_shutting_down() { return m_shutdown; }
  bool is_aborted() { return m_forced_shutdown; }

private:
  void run();

private:
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
};

#endif

/* EOF */
