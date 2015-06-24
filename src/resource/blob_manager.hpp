/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_RESOURCE_BLOB_MANAGER_HPP
#define HEADER_GALAPIX_RESOURCE_BLOB_MANAGER_HPP

#include <functional>
#include <set>

#include "resource/resource_locator.hpp"
#include "resource/blob_accessor.hpp"
#include "util/blob.hpp"
#include "util/failable.hpp"
#include "util/thread_pool.hpp"

class ArchiveManager;
class DownloadManager;
class ResourceLocator;

class BlobRequest
{
public:
  ResourceLocator locator;
  std::function<void (Failable<BlobPtr>)> callback;
};

class BlobManager
{
private:
  DownloadManager& m_download_mgr;
  ArchiveManager&  m_archive_mgr;
  ThreadPool m_pool;

  std::mutex m_mutex;
  std::vector<BlobRequest> m_requests;

public:
  BlobManager(DownloadManager& download_mgr,
              ArchiveManager& archive_mgr);
  ~BlobManager();

  void request_blob(const ResourceLocator& locator,
                    const std::function<void (Failable<BlobAccessorPtr>)>& callback);

private:
  void register_request(const ResourceLocator& locator);
  void unregister_request(const ResourceLocator& locator);

private:
  BlobManager(const BlobManager&) = delete;
  BlobManager& operator=(const BlobManager&) = delete;
};

#endif

/* EOF */
