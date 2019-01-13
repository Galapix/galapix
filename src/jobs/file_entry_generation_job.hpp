/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_JOBS_FILE_ENTRY_GENERATION_JOB_HPP
#define HEADER_GALAPIX_JOBS_FILE_ENTRY_GENERATION_JOB_HPP

#include <functional>
#include <boost/signals2/signal.hpp>

#include "util/url.hpp"
#include "job/job.hpp"
#include "database/entries/old_file_entry.hpp"
#include "galapix/tile.hpp"

class FileEntryGenerationJob : public Job
{
public:
  FileEntryGenerationJob(const JobHandle& job_handle, const URL& url);

  void run() override;

  boost::signals2::signal<void (OldFileEntry)>& sig_file_callback() { return m_sig_file_callback; }

private:
  URL m_url;
  boost::signals2::signal<void (OldFileEntry)> m_sig_file_callback;

private:
  FileEntryGenerationJob(const FileEntryGenerationJob&);
  FileEntryGenerationJob& operator=(const FileEntryGenerationJob&);
};

#endif

/* EOF */
