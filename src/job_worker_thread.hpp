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

#ifndef HEADER_JOB_WORKER_THREAD_HPP
#define HEADER_JOB_WORKER_THREAD_HPP

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include "thread_message_queue.hpp"
#include "thread.hpp"
#include "job_handle.hpp"

class Job;

class JobWorkerThread : public Thread
{
private:
  struct Task {
    Job* job;
    boost::function<void (Job*)> callback;
  };

  ThreadMessageQueue<Task> queue;
  bool quit;
  
public:
  JobWorkerThread();
  ~JobWorkerThread();

  JobHandle request(Job* job, const boost::function<void (Job*)>& callback);

  void run();

  void stop_thread();
  
private:
  JobWorkerThread (const JobWorkerThread&);
  JobWorkerThread& operator= (const JobWorkerThread&);
};

typedef boost::shared_ptr<JobWorkerThread> JobWorkerThreadHandle;

#endif

/* EOF */
