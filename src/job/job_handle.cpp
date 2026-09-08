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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <ostream>

#include "job/job_handle.hpp"

namespace galapix {

class JobHandleImpl
{
public:
  JobHandleImpl() :
    aborted(false),
    finished(false),
    failed(false),
    mutex(),
    cond()
  {}

public:
  // Atomically readable from any thread; writers still notify under mutex.
  std::atomic<bool> aborted;
  std::atomic<bool> finished;
  std::atomic<bool> failed;

  std::mutex     mutex;
  std::condition_variable cond;
};

JobHandle
JobHandle::create()
{
  return {};
}

JobHandle::JobHandle() :
  impl(std::make_unique<JobHandleImpl>())
{
}

JobHandle::~JobHandle()
{
}

void
JobHandle::set_aborted()
{
  impl->aborted.store(true, std::memory_order_release);
  std::lock_guard<std::mutex> lock(impl->mutex);
  impl->cond.notify_all();
}

bool
JobHandle::is_aborted() const
{
  return impl->aborted.load(std::memory_order_acquire);
}

void
JobHandle::set_finished()
{
  impl->finished.store(true, std::memory_order_release);
  std::lock_guard<std::mutex> lock(impl->mutex);
  impl->cond.notify_all();
}

bool
JobHandle::is_finished() const
{
  return impl->finished.load(std::memory_order_acquire) ||
         impl->aborted.load(std::memory_order_acquire);
}

void
JobHandle::set_failed()
{
  impl->finished.store(true, std::memory_order_release);
  impl->failed.store(true, std::memory_order_release);
  std::lock_guard<std::mutex> lock(impl->mutex);
  impl->cond.notify_all();
}

bool
JobHandle::is_failed() const
{
  return impl->failed.load(std::memory_order_acquire);
}

void
JobHandle::wait()
{
  std::unique_lock<std::mutex> lock(impl->mutex);
  impl->cond.wait(lock, [this] {
    return impl->finished.load(std::memory_order_acquire) ||
           impl->aborted.load(std::memory_order_acquire) ||
           impl->failed.load(std::memory_order_acquire);
  });
}

std::ostream& operator<<(std::ostream& os, JobHandle const& job_handle)
{
  return os << "JobHandle(this=" << job_handle.impl.get()
            << ", aborted=" << job_handle.is_aborted()
            << ", done=" << job_handle.is_finished() << ")";
}

} // namespace galapix

/* EOF */
