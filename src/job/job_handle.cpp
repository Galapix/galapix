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

#include <boost/thread/condition.hpp>

#include "job/job_handle.hpp"

class JobHandleImpl
{
public:
  JobHandleImpl()
    : aborted(false),
      finished(false),
      mutex(),
      cond()
  {}

  bool aborted;
  bool finished;

  boost::mutex     mutex;
  boost::condition cond;
};

JobHandle::JobHandle()
  : impl(new JobHandleImpl())
{
}

JobHandle::~JobHandle()
{
}

void
JobHandle::abort()
{
  boost::mutex::scoped_lock lock(impl->mutex);
  impl->aborted = true;
  impl->cond.notify_all();
}

bool
JobHandle::is_aborted() const
{
  return impl->aborted;
}

void
JobHandle::finish()
{
  boost::mutex::scoped_lock lock(impl->mutex);
  impl->finished = true;
  impl->cond.notify_all();
}

bool
JobHandle::is_finished() const
{
  return impl->finished;
}

void
JobHandle::wait()
{
  boost::mutex::scoped_lock lock(impl->mutex);
  if (!impl->finished)
    impl->cond.wait(lock);
}

/* EOF */
