/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_JOB_JOB_HANDLE_HPP
#define HEADER_GALAPIX_JOB_JOB_HANDLE_HPP

#include <memory>
#include <iosfwd>

class JobHandleImpl;

/** A JobHandle should be returend whenever one thread makes a request
    to another thread, the JobHandle allows the calling thread to
    cancel the job and the called thread to inform the calling one
    that the Job is finished. (FIXME: Do we need that last thing for something?) */
class JobHandle
{
  friend std::ostream& operator<<(std::ostream& os, const JobHandle& job_handle);

private:
  JobHandle();

public:
  static JobHandle create();
  ~JobHandle();

  /** Aborts a Job so that it gets removed from the JobManager without
      being called. */
  void set_aborted();
  bool is_aborted() const;

  /** Used by a Job to indicate that the Job is completed */
  void set_finished();
  bool is_finished() const;

  /** Reports that the Job couldn't be completed */
  void set_failed();
  bool is_failed() const;

  void wait();

private:
  std::shared_ptr<JobHandleImpl> impl;
};

std::ostream& operator<<(std::ostream& os, const JobHandle& job_handle);

#endif

/* EOF */
