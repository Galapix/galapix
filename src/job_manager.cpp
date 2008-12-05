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

#include <iostream>
#include "job.hpp"
#include "job_worker_thread.hpp"
#include "job_manager.hpp"

JobManager* JobManager::current_ = 0;

JobManager::JobManager(int num_threads)
  : next_thread(0)
{
  assert(current_ == 0);
  current_ = this;

  assert(num_threads > 0);

  for(int i = 0; i < num_threads; ++i)
    threads.push_back(new JobWorkerThread());
}

JobManager::~JobManager()
{
  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
    delete *i;
}

void 
JobManager::start_thread()
{
  boost::mutex::scoped_lock lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
    (*i)->start_thread();
}

void
JobManager::stop_thread()
{
  boost::mutex::scoped_lock lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
    (*i)->stop_thread();
}

void
JobManager::join_thread()
{
  boost::mutex::scoped_lock lock(mutex);

  for(Threads::iterator i = threads.begin(); i != threads.end(); ++i)
    (*i)->join_thread();
}

JobHandle
JobManager::request(Job* job, const boost::function<void (Job*)>& callback)
{
  boost::mutex::scoped_lock lock(mutex);

  JobHandle handle = job->get_handle();
  threads[next_thread]->request(job, callback);
  
  next_thread += 1;
  if (next_thread >= threads.size())
    next_thread = 0;

  return handle;
}

/* EOF */
