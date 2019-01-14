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

#ifndef HEADER_GALAPIX_JOB_JOB_HANDLE_GROUP_HPP
#define HEADER_GALAPIX_JOB_JOB_HANDLE_GROUP_HPP

#include <vector>

#include "job/job_handle.hpp"

class JobHandleGroup
{
public:
  typedef std::vector<JobHandle> JobHandles;

public:
  JobHandleGroup() :
    m_job_handles()
  {}

  void add(JobHandle const& job_handle)
  {
    m_job_handles.push_back(job_handle);
  }

  void wait()
  {
    for(JobHandles::iterator i = m_job_handles.begin(); i != m_job_handles.end(); ++i)
    {
      i->wait();
    }
  }

  void clear()
  {
    m_job_handles.clear();
  }

private:
  JobHandles m_job_handles;

private:
  JobHandleGroup(const JobHandleGroup&);
  JobHandleGroup& operator=(const JobHandleGroup&);
};

#endif

/* EOF */
