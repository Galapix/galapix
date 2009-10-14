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

#ifndef HEADER_GALAPIX_JOB_JOB_MANAGER_HPP
#define HEADER_GALAPIX_JOB_JOB_MANAGER_HPP

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

#include "job/job_handle.hpp"

class JobWorkerThread;
class Job;

class JobManager
{
private:
  typedef std::vector<boost::shared_ptr<JobWorkerThread> > Threads;
  Threads threads;
  Threads::size_type next_thread;

  boost::mutex mutex;

public:
  JobManager(int num_threads);
  ~JobManager();

  void start_thread();
  void stop_thread();
  void abort_thread();
  void join_thread();

  void wait();

  /** \a job is processed and once finished \a callback is called, \a
      job will be deleted afterwards */
  JobHandle request(boost::shared_ptr<Job> job, 
                    const boost::function<void (boost::shared_ptr<Job>, bool)>& callback = boost::function<void (boost::shared_ptr<Job>, bool)>());
};

#endif

/* EOF */
