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

#include "jobs/file_entry_generation_job.hpp"

#include <logmich/log.hpp>
#include <surf/software_surface_factory.hpp>

#include "jobs/tile_generator.hpp"
#include "plugins/jpeg.hpp"
#include "util/filesystem.hpp"

FileEntryGenerationJob::FileEntryGenerationJob(const JobHandle& job_handle, const URL& url) :
  Job(job_handle),
  m_url(url),
  m_sig_file_callback()
{
}

void
FileEntryGenerationJob::run()
{
  try
  {
#if 0
    SoftwareSurface surface;
    Size size;
    int min_scale;
    int max_scale;

    // FIXME: JPEG::filename_is_jpeg() is ugly
    if (!m_url.is_remote() && JPEG::filename_is_jpeg(m_url.str()))
    {
      Blob blob;

      if (m_url.has_stdio_name())
      {
        size = JPEG::get_size(m_url.get_stdio_name());
      }
      else
      {
        blob = m_url.get_blob();
        size = JPEG::get_size(blob.get_data(), blob.size());
      }

      // FIXME: On http:// transfer mtime and size must be got from the transfer itself, not afterwards
      file_entry = FileEntry(RowId(), m_url, m_url.get_size(), m_url.get_mtime(), FileEntry::kUnknownHandler);

      // 2^3 is the highest scale JPEG supports, so we limit the
      // min_scale to that
      min_scale = Math::min(min_scale, 3);

      // FIXME: recalc min_scale from jpeg scale
      if (!blob)
      {
        surface = JPEG::load_from_file(m_url.get_stdio_name(), Math::pow2(min_scale));
      }
      else
      {
        surface = JPEG::load_from_mem(blob.get_data(), blob.size(), Math::pow2(min_scale));
      }
    }
    else
    {
      // FIXME: On http:// transfer mtime and size must be got from the transfer itself, not afterwards
      surface = SoftwareSurfaceFactory::current().from_url(m_url);
      size = surface->get_size();
      file_entry = FileEntry(RowId(), m_url, m_url.get_size(), m_url.get_mtime(), FileEntry::kImageHandler);
      min_scale = 0;
      max_scale = file_entry.get_thumbnail_scale();
    }

    m_sig_file_callback(file_entry);

    TileGenerator::cut_into_tiles(surface, size, min_scale, max_scale,
                                  [this](const Tile& tile){
                                    m_sig_tile_callback(tile);
                                  });
#endif
  }
  catch(const std::exception& err)
  {
    log_error("Error while processing {}", m_url);
    log_error("  Exception: {}", err.what());
  }
}

/* EOF */
