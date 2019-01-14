/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include "job/job_manager.hpp"

#include <unistd.h>

#include "job/job.hpp"
#include "job/job_worker_thread.hpp"

JobManager::JobManager(int num_threads) :
  threads(),
  next_thread(0),
  mutex()
{
  assert(num_threads > 0);

  for(int i = 0; i < num_threads; ++i)
  {
    threads.push_back(std::make_shared<JobWorkerThread>());
  }
}

JobManager::~JobManager()
{
}

void
JobManager::start_thread()
{
  std::unique_lock<std::mutex> lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
  {
    (*i)->start_thread();
  }
}

void
JobManager::stop_thread()
{
  std::unique_lock<std::mutex> lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
  {
    (*i)->stop_thread();
  }
}

void
JobManager::abort_thread()
{
  std::unique_lock<std::mutex> lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
  {
    (*i)->abort_thread();
  }
}

void
JobManager::join_thread()
{
  std::unique_lock<std::mutex> lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
  {
    (*i)->join_thread();
  }
}

JobHandle
JobManager::request(std::shared_ptr<Job> const& job,
                    std::function<void (std::shared_ptr<Job>, bool)> const& callback)
{
  std::unique_lock<std::mutex> lock(mutex);

  JobHandle handle = job->get_handle();
  threads[next_thread]->request(job, callback);

  next_thread += 1;
  if (next_thread >= threads.size())
  {
    next_thread = 0;
  }

  return handle;
}

/* EOF */
