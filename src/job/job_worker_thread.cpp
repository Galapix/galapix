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

#include "job/job_worker_thread.hpp"

#include "job/job.hpp"

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
    m_queue.wait();

    while(!m_abort && !m_queue.empty())
    {
      Task task = m_queue.front();
      m_queue.pop();

      if (!task.job->is_aborted())
      {
        //std::cout << "start job: " << task.job << std::endl;
        task.job->run();
        if (task.callback)
        {
          task.callback(task.job, true);
        }
        task.job->get_handle().finish();
        //std::cout << "done job: " << task.job << std::endl;
      }
      else
      {
        task.callback(task.job, false);
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
JobWorkerThread::request(boost::shared_ptr<Job> job, const boost::function<void (boost::shared_ptr<Job>, bool)>& callback)
{
  JobHandle handle = job->get_handle();
  
  Task task;
  task.job      = job;
  task.callback = callback;
  m_queue.push(task);

  return handle;
}

/* EOF */
