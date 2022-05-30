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

#include "galapix/thumbnail_generator.hpp"

#include "galapix/options.hpp"
#include "job/job_handle_group.hpp"

ThumbnailGenerator::ThumbnailGenerator(Options const& opts) :
  m_database(Database::create(opts.database)),
  m_job_manager(opts.threads),
  m_database_thread(m_database, m_job_manager)
{
  m_database_thread.start_thread();
  m_job_manager.start_thread();
}

ThumbnailGenerator::~ThumbnailGenerator()
{
  try
  {
    m_job_manager.stop_thread();
    m_database_thread.stop_thread();

    m_job_manager.join_thread();
    m_database_thread.join_thread();
  }
  catch(std::exception const& err)
  {
    log_error(err.what());
  }
}

void
ThumbnailGenerator::run(std::vector<URL> const& urls, bool generate_all_tiles)
{
  std::vector<OldFileEntry> file_entries;

  JobHandleGroup job_handle_group;

  // gather FileEntries
  for(auto const& url : urls)
  {
    job_handle_group.add(m_database_thread.request_file(url,
                                                        [&file_entries](OldFileEntry const& entry) {
                                                          file_entries.push_back(entry);
                                                        }));
  }
  job_handle_group.wait();
  job_handle_group.clear();

  std::cout << "Got " << file_entries.size() << " files, generating tiles...: "  << generate_all_tiles << std::endl;

  // gather thumbnails
  for(std::vector<OldFileEntry>::const_iterator i = file_entries.begin(); i != file_entries.end(); ++i)
  {
    int min_scale = 0;
    int max_scale = 1; // FIXME: i->get_thumbnail_scale();

    if (!generate_all_tiles)
    {
      min_scale = std::max(0, max_scale - 3);
    }

    job_handle_group.add(m_database_thread.request_tiles(*i, min_scale, max_scale,
                                                         std::function<void(Tile)>()));
  }

  job_handle_group.wait();
  job_handle_group.clear();
}

/* EOF */
