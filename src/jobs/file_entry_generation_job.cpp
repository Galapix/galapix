/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#include <boost/bind.hpp>

#include "util/software_surface_factory.hpp"
#include "util/log.hpp"
#include "jobs/tile_generator.hpp"

FileEntryGenerationJob::FileEntryGenerationJob(const JobHandle& job_handle, const URL& url) :
  Job(job_handle),
  m_url(url),
  m_sig_file_callback(),
  m_sig_tile_callback()
{
}

void
FileEntryGenerationJob::run()
{
  // generate FileEntry if not already given
  Size size;
  if (!SoftwareSurfaceFactory::get_size(m_url, size))
  {
    log_warning << "Couldn't get size for " << m_url << std::endl;
    return;
  }
  else
  {
    FileEntry file_entry = FileEntry::create_without_fileid(m_url, m_url.get_size(), m_url.get_mtime(), 
                                                            size.width, size.height);
    m_sig_file_callback(file_entry);

    // FIXME: here we are just guessing which tiles might be useful
    int min_scale = std::max(0, file_entry.get_thumbnail_scale() - 3);
    int max_scale = file_entry.get_thumbnail_scale();

    try 
    {
      // Do the main work
      TileGenerator::generate(m_url, min_scale, max_scale, 
                              boost::bind(&FileEntryGenerationJob::process_tile, this, file_entry, _1));
    }
    catch(const std::exception& err)
    {
      log_error << "Error while processing " << file_entry << std::endl;
      log_error << "  Exception: " << err.what() << std::endl;
    }
  }
}

void
FileEntryGenerationJob::process_tile(const FileEntry& file_entry, const Tile& tile)
{
  m_sig_tile_callback(file_entry, tile);
}

/* EOF */
