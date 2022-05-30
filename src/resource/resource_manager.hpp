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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_MANAGER_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_MANAGER_HPP

#include <functional>

#include <arxp/fwd.hpp>

#include "job/thread.hpp"
#include "job/thread_message_queue2.hpp"
#include "util/async_messenger.hpp"
#include "util/blob.hpp"
#include "util/failable.hpp"

namespace galapix {

class ArchiveInfo;
class BlobInfo;
class DatabaseThread;
class DownloadManager;
class FileInfo;
class Generator;
class ImageInfo;
class ResourceData;
class ResourceInfo;
class ResourceLocator;
class ResourceMetadata;
class ResourceName;
class TileInfo;
class URLInfo;

class ResourceManager
{
public:
  ResourceManager(DatabaseThread& database,
                  Generator& generator,
                  DownloadManager& download_mgr,
                  arxp::ArchiveManager& archive_mgr);
  ~ResourceManager();

  void request_resource_metadata(ResourceLocator const& locator,
                                 const std::function<void (Failable<ResourceMetadata> const&)>& callback);

  void request_tile_info(ImageInfo const& image, int scale, int x, int y,
                         const std::function<void (Failable<TileInfo> const&)>& callback);

  void request_resource_info(ResourceLocator const& locator, BlobInfo const& blob,
                             const std::function<void (Failable<ResourceInfo>)>& callback);

private:
  DatabaseThread& m_database;
  Generator& m_generator;
  DownloadManager& m_download_mgr;
  arxp::ArchiveManager& m_archive_mgr;

private:
  ResourceManager(ResourceManager const&) = delete;
  ResourceManager& operator=(ResourceManager const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
