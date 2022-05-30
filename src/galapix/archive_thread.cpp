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

#include "galapix/archive_thread.hpp"

#include <logmich/log.hpp>

namespace galapix {

ArchiveThread::ArchiveThread(std::string const& tmpdir) :
  m_archive_mgr(tmpdir),
  m_mutex(),
  m_extractions(),
  m_pool(2)
{
}

ArchiveThread::~ArchiveThread()
{
}

void
ArchiveThread::request_blob(std::string const& archive_filename, std::string const& filename,
                            const std::function<void (Failable<Blob>)>& callback)
{
  m_pool.schedule
    ([this, archive_filename, filename, callback]
     {
       try
       {
         std::vector<uint8_t> blob = m_archive_mgr.get_file(archive_filename, filename);
         callback(Blob::copy(std::move(blob)));
       }
       catch(...)
       {
         callback(Failable<Blob>(std::current_exception()));
       }
     });
}

void
ArchiveThread::request_extraction(std::string const& archive_filename,
                                  const std::function<void (Failable<arxp::ExtractionPtr>)>& callback)
{
  m_pool.schedule
    ([this, archive_filename, callback]
     {
       try
       {
         std::unique_lock<std::mutex> extraction_lock;
         ExtractionEntry& entry = get_and_lock_extraction_entry(archive_filename, extraction_lock);
         if (entry.extraction)
         {
           log_debug("request_extraction: from_cache: {}", archive_filename);
           callback(entry.extraction);
         }
         else
         {
           log_debug("request_extraction: generating: {}", archive_filename);
           entry.extraction = m_archive_mgr.get_extraction(archive_filename);
           callback(entry.extraction);
         }
       }
       catch(...)
       {
         callback(Failable<arxp::ExtractionPtr>(std::current_exception()));
       }
     });
}

ArchiveThread::ExtractionEntry&
ArchiveThread::get_and_lock_extraction_entry(std::string const& archive_filename,
                                             std::unique_lock<std::mutex>& lock_out)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  auto it = m_extractions.find(archive_filename);
  if (it == m_extractions.end())
  {
    auto entry_uptr = std::make_unique<ExtractionEntry>();

    ExtractionEntry& entry = *entry_uptr;
    m_extractions[archive_filename] = std::move(entry_uptr);

    lock_out = std::unique_lock<std::mutex>(entry.mutex);
    return entry;
  }
  else
  {
    lock_out = std::unique_lock<std::mutex>(it->second->mutex);
    return *it->second;
  }
}

} // namespace galapix

/* EOF */
