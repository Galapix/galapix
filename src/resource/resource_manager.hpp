/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GALAPIX_RESOURCE_MANAGER_HPP
#define HEADER_GALAPIX_GALAPIX_RESOURCE_MANAGER_HPP

#include <functional>

#include "job/thread.hpp"
#include "job/thread_message_queue2.hpp"
#include "util/async_messenger.hpp"
#include "util/blob.hpp"
#include "util/failable.hpp"
#include "util/currenton.hpp"

class ArchiveInfo;
class ArchiveManager;
class DatabaseThread;
class DownloadManager;
class FileInfo;
class Generator;
class ImageInfo;
class ResourceInfo;
class ResourceLocator;
class TileInfo;
class URLInfo;

class ResourceManager : //public AsyncMessenger,
                        public Currenton<ResourceManager>
{
private:
  DatabaseThread& m_database;
  Generator& m_generator;
  DownloadManager& m_download_mgr;
  ArchiveManager& m_archive_mgr;
  
public:
  ResourceManager(DatabaseThread& database,
                  Generator& generator,
                  DownloadManager& download_mgr,
                  ArchiveManager& archive_mgr);
  virtual ~ResourceManager();

  void request_info(const ResourceLocator& locator, 
                    const std::function<void (Failable<ResourceInfo>)>& callback);

  void request_blob(const ResourceLocator& locator, 
                    const std::function<void (Failable<BlobPtr>)>& callback);

  void request_file_info(const ResourceLocator& locator, 
                         const std::function<void (Failable<FileInfo>)>& callback);
  void request_url_info(const ResourceLocator& locator, 
                         const std::function<void (Failable<URLInfo>)>& callback);

  void request_resource_info(const ResourceLocator& locator,
                             const std::function<void (const Failable<ResourceInfo>&)>& callback);
  void request_archive_info(const ResourceInfo& resource,
                            const std::function<void (const Failable<ArchiveInfo>&)>& callback);
  void request_image_info(const ResourceInfo& resource,
                          const std::function<void (const Failable<ImageInfo>&)>& callback);
  void request_tile_info(const ImageInfo& image, int scale, int x, int y,
                         const std::function<void (const Failable<TileInfo>&)>& callback);
  
private:
  ResourceManager(const ResourceManager&) = delete;
  ResourceManager& operator=(const ResourceManager&) = delete;
};

#endif

/* EOF */
