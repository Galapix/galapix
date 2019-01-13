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

#ifndef HEADER_GALAPIX_JOB_JOB_HPP
#define HEADER_GALAPIX_JOB_JOB_HPP

#include "job/job_handle.hpp"

class Job
{
public:
  Job(JobHandle const& handle) : m_handle(handle) {}
  virtual ~Job() {}

  virtual void run() =0;
  virtual bool is_aborted() { return m_handle.is_aborted(); }

  JobHandle get_handle() const { return m_handle; }

private:
  JobHandle m_handle;

private:
  Job (const Job&);
  Job& operator= (const Job&);
};

typedef std::shared_ptr<Job> JobPtr;

#endif

/* EOF */
