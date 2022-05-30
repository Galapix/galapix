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

#include <arxp/archive_manager.hpp>
#include <arxp/extraction.hpp>

#include "util/blob.hpp"
#include "util/failable.hpp"
#include "util/thread_pool.hpp"

namespace galapix {

class ArchiveThread final
{
private:
  struct ExtractionEntry
  {
    std::mutex    mutex;
    arxp::ExtractionPtr extraction;

    ExtractionEntry() :
      mutex(),
      extraction()
    {}
  };

public:
  ArchiveThread(std::string const& tmpdir);
  ~ArchiveThread();

  void request_blob(std::string const& archive_filename, std::string const& filename,
                    const std::function<void (Failable<Blob>)>& callback);
  void request_extraction(std::string const& archive_filename,
                          const std::function<void (Failable<arxp::ExtractionPtr>)>& callback);

private:
  ExtractionEntry& get_and_lock_extraction_entry(std::string const& archive_filename,
                                                 std::unique_lock<std::mutex>& lock_out);

private:
  arxp::ArchiveManager m_archive_mgr;
  std::mutex m_mutex;
  std::unordered_map<std::string, std::unique_ptr<ExtractionEntry> > m_extractions;
  ThreadPool m_pool;

private:
  ArchiveThread(ArchiveThread const&) = delete;
  ArchiveThread& operator=(ArchiveThread const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
