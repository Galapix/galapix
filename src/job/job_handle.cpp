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

#include <condition_variable>
#include <mutex>
#include <ostream>

#include "job/job_handle.hpp"

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
  bool aborted;
  bool finished;
  bool failed;

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
  std::lock_guard<std::mutex> lock(impl->mutex);
  impl->aborted = true;
  impl->cond.notify_all();
}

bool
JobHandle::is_aborted() const
{
  return impl->aborted;
}

void
JobHandle::set_finished()
{
  std::lock_guard<std::mutex> lock(impl->mutex);
  impl->finished = true;
  impl->cond.notify_all();
}

bool
JobHandle::is_finished() const
{
  return impl->finished || impl->aborted;
}

void
JobHandle::set_failed()
{
  std::lock_guard<std::mutex> lock(impl->mutex);
  impl->finished = true;
  impl->failed   = true;
  impl->cond.notify_all();
}

bool
JobHandle::is_failed() const
{
  return impl->failed;
}

void
JobHandle::wait()
{
  std::unique_lock<std::mutex> lock(impl->mutex);
  if (!impl->finished && !impl->aborted && !impl->failed)
  {
    impl->cond.wait(lock, [this]{ return is_finished(); });
  }
}

std::ostream& operator<<(std::ostream& os, JobHandle const& job_handle)
{
  return os << "JobHandle(this=" << job_handle.impl.get()
            << ", aborted=" << job_handle.is_aborted()
            << ", done=" << job_handle.is_finished() << ")";
}

/* EOF */
