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

JobWorkerThread::JobWorkerThread()
  : quit(false)
{
}

JobWorkerThread::~JobWorkerThread()
{
  assert(quit);

  while(!queue.empty())
    {
      Task task = queue.front();
      queue.pop();
    }
}

void
JobWorkerThread::run()
{
  while(!quit)
    {
      queue.wait();

      while(!queue.empty())
        {
          Task task = queue.front();
          queue.pop();

          if (!task.job->get_handle().is_aborted())
            {
              //std::cout << "start job: " << task.job << std::endl;
              task.job->run();
              if (task.callback)
                task.callback(task.job);
              task.job->get_handle().finish();
              //std::cout << "done job: " << task.job << std::endl;
            }
        }
    }
}

void
JobWorkerThread::stop_thread()
{
  quit = true;
  queue.wakeup();  
}

JobHandle
JobWorkerThread::request(boost::shared_ptr<Job> job, const boost::function<void (boost::shared_ptr<Job>)>& callback)
{
  JobHandle handle = job->get_handle();
  
  Task task;
  task.job      = job;
  task.callback = callback;
  queue.push(task);

  return handle;
}

/* EOF */
