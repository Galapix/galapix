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

#include "resource/resource_manager.hpp"

#include <future>
#include <logmich/log.hpp>
#include <fmt/format.h>

#include "generator/generator.hpp"
#include "network/download_manager.hpp"
#include "network/download_result.hpp"
#include "resource/archive_info.hpp"
#include "resource/image_info.hpp"
#include "resource/resource_generator_callbacks.hpp"
#include "resource/resource_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_metadata.hpp"
#include "resource/resource_url.hpp"
#include "resource/tile_info.hpp"
#include "resource/url_info.hpp"
#include "server/database_thread.hpp"

ResourceManager::ResourceManager(DatabaseThread& database,
                                 Generator& generator,
                                 DownloadManager& download_mgr,
                                 arxp::ArchiveManager& archive_mgr) :
  m_database(database),
  m_generator(generator),
  m_download_mgr(download_mgr),
  m_archive_mgr(archive_mgr)
{
}

ResourceManager::~ResourceManager()
{
}

#if 0
void
ResourceManager::request_resource_metadata(const ResourceLocator& locator,
                                           const std::function<void (const Failable<ResourceMetadata>&)>& callback)
{
  m_database.request_resource_info
    (locator, blob,
     [this, locator, blob, callback](const std::optional<ResourceInfo>& reply)
     {
       if (reply)
       {
         callback(*reply);
       }
       else
       {
         m_generator.request_resource_processing
           (locator, std::make_shared<ResourceGeneratorCallbacks>(locator));

         callback(Failable<ResourceInfo>::from_exception(
                    std::runtime_error("ResourceManager::request_resource_info(): not implemented")));
       }
     });
}
#endif

void
ResourceManager::request_resource_info(const ResourceLocator& locator, const BlobInfo& blob,
                                       const std::function<void (Failable<ResourceInfo>)>& callback)
{
  m_database.request_resource_info
    (locator, blob,
     [this, locator, callback](const std::optional<ResourceInfo>& reply)
     {
       if (reply)
       {
         callback(*reply);
       }
       else
       {
         m_generator.request_resource_processing
           (locator, std::make_shared<ResourceGeneratorCallbacks>(locator));

         callback(Failable<ResourceInfo>::from_exception(
                    std::runtime_error("ResourceManager::request_resource_info(): not implemented")));
       }
     });
}

void
ResourceManager::request_resource_metadata(const ResourceLocator& locator,
                                           const std::function<void (const Failable<ResourceMetadata>&)>& callback)
{
#if 0
  if (locator.get_url().get_scheme() == "file")
  {
    request_file_info
      (locator.get_url().get_path(),
       [this, locator, callback](const Failable<FileInfo>& data)
       {
         try
         {
           const FileInfo& file_info = data.get();
           request_resource_metadata(locator, file_info.get_blob_info(), callback);
         }
         catch(...)
         {
           callback(Failable<ResourceInfo>(std::current_exception()));
         }
       });
  }
  else if (locator.get_url().is_remote())
  {
    request_url_info
      (locator.get_url().str(),
       [this, locator, callback](const Failable<URLInfo>& data)
       {
         try
         {
           const URLInfo& url_info = data.get();
           request_resource_info(locator, url_info.get_blob_info(), callback);
         }
         catch(...)
         {
           callback(Failable<ResourceInfo>(std::current_exception()));
         }
       });
  }
  else
  {
    Failable<ResourceInfo> result;
    result.set_exception(std::make_exception_ptr(std::runtime_error("unknown URL schema")));
    callback(result);
  }
#endif
}

#if 0
void
ResourceManager::request_image_info(const ResourceInfo& resource,
                                    const std::function<void (const Failable<ImageInfo>&)>& callback)
{
  m_database.request_image_info
    (resource,
     [this, resource, callback](const std::optional<ImageInfo>& image_info)
     {
       if (image_info)
       {
         callback(*image_info);
       }
       else
       {
         /*
         m_generator.request_resource_processing(resource,
            // full generation or incremental generation
            [this, callback](const Failable<ImageInfo>& result)
            {

            });
         */
       }
     });
}

void
ResourceManager::request_archive_info(const ResourceInfo& resource,
                                      const std::function<void (const Failable<ArchiveInfo>&)>& callback)
{
}

void
ResourceManager::request_tile_info(const ImageInfo& image, int scale, int x, int y,
                                   const std::function<void (const Failable<TileInfo>&)>& callback)
{
#if 0
  m_database.request_tile(
    image.get_rowid(), scale, x, y,
    [this](const std::optional<TileEntry>& tile_entry)
    {
      if (!tile_entry)
      {
        generate_tiles(
          image, scale,
          [this]()
          {

          });
      }
      else
      {
        callback(TileInfo(tile_entry));
      }
    });
#endif
}
#endif

#if 0
void
ResourceManager::generate_tiles()
{
  // tile is not in the database, so request it's generation
  m_generator.request_tiling(
    image, scale,
    [this]()
    {
      // once generation is complete, rerequest the tile
      m_database.request_tile(
        image.get_rowid(), scale, x, y,
        [this](const std::optional<TileEntry>& tile_entry)
        {
          callback(TileInfo(tile_entry));
        });
    });
}
#endif

/* EOF */
