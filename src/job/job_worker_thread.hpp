/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_JOB_JOB_WORKER_THREAD_HPP
#define HEADER_GALAPIX_JOB_JOB_WORKER_THREAD_HPP

#include <functional>
#include <memory>

#include "job/thread_message_queue2.hpp"
#include "job/thread.hpp"
#include "job/job_handle.hpp"

class Job;

class JobWorkerThread : public Thread
{
private:
  struct Task
  {
    std::shared_ptr<Job> job;
    std::function<void (std::shared_ptr<Job>, bool)> callback;

    Task() :
      job(),
      callback()
    {}
  };

public:
  JobWorkerThread();
  ~JobWorkerThread() override;

  JobHandle request(std::shared_ptr<Job> const& job, const std::function<void (std::shared_ptr<Job>, bool)>& callback);

  void run() override;

  void stop_thread() override;
  void abort_thread();

  bool empty() { return m_queue.empty(); }

private:
  ThreadMessageQueue2<Task> m_queue;
  bool m_quit;
  bool m_abort;

private:
  JobWorkerThread (const JobWorkerThread&);
  JobWorkerThread& operator= (const JobWorkerThread&);
};

typedef std::shared_ptr<JobWorkerThread> JobWorkerThreadPtr;

#endif

/* EOF */
