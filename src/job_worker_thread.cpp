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

#include "job.hpp"
#include "job_worker_thread.hpp"

JobWorkerThread::JobWorkerThread()
 : quit(false)
{
}

int
JobWorkerThread::run()
{
  while(!quit)
    {
      while(!queue.empty())
        {
          Task task = queue.front();
          queue.pop();

          if (!task.job->get_handle().is_aborted())
            {
              task.job->run();
              if (task.callback)
                task.callback(task.job);
            }

          delete task.job;
        }
      queue.wait();
    }

  return 0;
}

void
JobWorkerThread::stop()
{
  quit = true;
  queue.wakeup();
}

void
JobWorkerThread::finish()
{
  quit = true;
  queue.wakeup();  
}

JobHandle
JobWorkerThread::request(Job* job, const boost::function<void (Job*)>& callback)
{
  Task task;
  task.job      = job;
  task.callback = callback;
  queue.push(task);
  
  return job->get_handle();
}

/* EOF */
