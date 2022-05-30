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

#include "job/job_worker_thread.hpp"

#include <iostream>

#include "job/job.hpp"

namespace galapix {

JobWorkerThread::JobWorkerThread()
  : m_queue(),
    m_quit(false),
    m_abort(false)
{
}

JobWorkerThread::~JobWorkerThread()
{
  assert(m_quit);
}

void
JobWorkerThread::run()
{
  while(!m_quit)
  {
    m_queue.wait_for_pop([this]{ return m_abort || m_quit; });

    Task task;
    while(!m_abort && m_queue.try_pop(task))
    {
      // std::cout << "JobWorkerThread::run(): " << this << " size: " << m_queue.size() << std::endl;
      if (!task.job->is_aborted())
      {
        //std::cout << "start job: " << task.job << std::endl;
        try
        {
          task.job->run();
        }
        catch(std::exception const& err)
        {
          std::cout << "JobWorkerThread:run: Job failed: " << err.what() << std::endl;
        }

        if (task.callback)
        {
          task.callback(task.job, true);
        }

        // FIXME: Do something to check that the JobHandle is in is_finished() state
        //if (task.job->get_handle().is_finished();
        //std::cout << "done job: " << task.job << std::endl;
      }
      else
      {
        if (task.callback)
        {
          task.callback(task.job, false);
        }
      }
    }
  }
}

void
JobWorkerThread::abort_thread()
{
  m_quit = true;
  m_abort = true;
  m_queue.wakeup();
}

void
JobWorkerThread::stop_thread()
{
  m_quit = true;
  m_queue.wakeup();
}

JobHandle
JobWorkerThread::request(std::shared_ptr<Job> const& job, const std::function<void (std::shared_ptr<Job>, bool)>& callback)
{
  JobHandle handle = job->get_handle();

  Task task;
  task.job      = job;
  task.callback = callback;
  m_queue.wait_and_push(task);

  return handle;
}

} // namespace galapix

/* EOF */
