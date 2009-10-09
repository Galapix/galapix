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

#include "jobs/file_entry_generation_job.hpp"

#include <assert.h>

#include "galapix/database_thread.hpp"
#include "util/software_surface_factory.hpp"

FileEntryGenerationJob::FileEntryGenerationJob(const JobHandle& job_handle, const URL& url,
                                               const boost::function<void (const FileEntry&)>& callback)
  : m_job_handle(job_handle),
    m_url(url),
    m_callback(callback)
{
  assert(m_callback);
}

void
FileEntryGenerationJob::run()
{
  Size size;
  if (SoftwareSurfaceFactory::get_size(m_url, size))
  {
    DatabaseThread::current()->store_file_entry(m_url, size, m_callback);
  }
  else
  {
    m_callback(FileEntry());
  }
}

/* EOF */
