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

#ifndef HEADER_JOB_HPP
#define HEADER_JOB_HPP

#include "job_handle.hpp"

class Job
{
private:
  JobHandle m_handle;

public:
  Job() {}
  Job(JobHandle handle) : m_handle(handle) {}
  virtual ~Job() {}

  JobHandle get_handle() const { return m_handle; }

  virtual void run() =0;

private:
  Job (const Job&);
  Job& operator= (const Job&);
};

#endif

/* EOF */
