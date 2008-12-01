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

#ifndef HEADER_JOB_MANAGER_HPP
#define HEADER_JOB_MANAGER_HPP

#include <boost/function.hpp>
#include <vector>
#include "job_handle.hpp"

class JobWorkerThread;
class Job;

class JobManager
{
private:
  static JobManager* current_; 
public:
  static JobManager* current() { return current_; }

private:
  typedef std::vector<JobWorkerThread*> Threads;
  Threads threads;
  Threads::size_type next_thread;

public:
  JobManager(int num_threads);
  ~JobManager();

  /** Stops the worker thread at the next possible point */
  void stop();

  void join();

  /** Waits till all jobs in the queue are finished, then returns */
  void finish();

  JobHandle request(Job* job, const boost::function<void (Job*)>& callback);
};

#endif

/* EOF */
