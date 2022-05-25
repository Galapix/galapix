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

#ifndef HEADER_GALAPIX_ARCHIVE_ARCHIVE_THREAD_HPP
#define HEADER_GALAPIX_ARCHIVE_ARCHIVE_THREAD_HPP

#include <functional>
#include <unordered_map>
#include <mutex>
#include <string>

#include "arch/archive_manager.hpp"
#include "arch/extraction.hpp"
#include "util/blob.hpp"
#include "util/failable.hpp"
#include "util/thread_pool.hpp"

class ArchiveThread final
{
private:
  struct ExtractionEntry
  {
    std::mutex    mutex;
    ExtractionPtr extraction;

    ExtractionEntry() :
      mutex(),
      extraction()
    {}
  };

public:
  ArchiveThread(const std::string& tmpdir);
  ~ArchiveThread();

  void request_blob(const std::string& archive_filename, const std::string& filename,
                    const std::function<void (Failable<Blob>)>& callback);
  void request_extraction(const std::string& archive_filename,
                          const std::function<void (Failable<ExtractionPtr>)>& callback);

private:
  ExtractionEntry& get_and_lock_extraction_entry(const std::string& archive_filename,
                                                 std::unique_lock<std::mutex>& lock_out);

private:
  ArchiveManager m_archive_mgr;
  std::mutex m_mutex;
  std::unordered_map<std::string, std::unique_ptr<ExtractionEntry> > m_extractions;
  ThreadPool m_pool;

private:
  ArchiveThread(const ArchiveThread&) = delete;
  ArchiveThread& operator=(const ArchiveThread&) = delete;
};

#endif

/* EOF */
