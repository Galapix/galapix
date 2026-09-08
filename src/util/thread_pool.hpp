// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_UTIL_THREAD_POOL_HPP
#define HEADER_GALAPIX_UTIL_THREAD_POOL_HPP

#include <functional>
#include <atomic>
#include <thread>
#include <vector>

#include "job/thread_message_queue2.hpp"

namespace galapix {

class ThreadPool
{
public:
  using Task = std::function<void ()>;

  enum ShutdownPolicy {
    kWaitForAllTasks,
    kWaitForActiveTasks
  };

public:
  ThreadPool(int num_threads = 4, ShutdownPolicy shutdown_policy = kWaitForAllTasks);
  ~ThreadPool();

  void schedule(Task const& task);
  void abort();

  bool is_shutting_down() const { return m_shutdown.load(std::memory_order_acquire); }
  bool is_aborted() const { return m_forced_shutdown.load(std::memory_order_acquire); }

private:
  void run();

private:
  ShutdownPolicy m_shutdown_policy;
  std::atomic<bool> m_shutdown;
  std::atomic<bool> m_forced_shutdown;
  std::vector<std::thread> m_threads;

  ThreadMessageQueue2<Task> m_queue;

private:
  ThreadPool(ThreadPool const&) = delete;
  ThreadPool& operator=(ThreadPool const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
