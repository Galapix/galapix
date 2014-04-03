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

#include "resource/resource_manager.hpp"

#include <future>

#include "galapix/database_thread.hpp"
#include "generator/generator.hpp"
#include "network/download_manager.hpp"
#include "network/download_result.hpp"
#include "resource/archive_info.hpp"
#include "resource/image_info.hpp"
#include "resource/resource_generator_callbacks.hpp"
#include "resource/resource_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_url.hpp"
#include "resource/tile_info.hpp"
#include "resource/url_info.hpp"
#include "util/format.hpp"
#include "util/log.hpp"

ResourceManager::ResourceManager(DatabaseThread& database,
                                 Generator& generator,
                                 DownloadManager& download_mgr,
                                 ArchiveManager& archive_mgr) :
  m_database(database),
  m_generator(generator),
  m_download_mgr(download_mgr),
  m_archive_mgr(archive_mgr)
{
}

ResourceManager::~ResourceManager()
{
}

void
ResourceManager::request_blob_info(const ResourceLocator& locator,
                                   const std::function<void (Failable<BlobInfo>)>& callback)
{
  ResourceLocator blob_locator = locator.get_blob_locator();
  if (blob_locator.get_url().is_remote())
  {
    request_url_info(locator.get_url().get_path(),
                     [callback](const Failable<URLInfo>& url_info)
                     {
                       try
                       {
                         callback(url_info.get().get_blob_info());
                       }
                       catch(...)
                       {
                         callback(Failable<BlobInfo>(std::current_exception()));
                       }
                     });
  }
  else
  {
    request_file_info(locator.get_url().get_path(),
                      [callback](const Failable<FileInfo>& file_info)
                      {
                        try
                        {
                          callback(file_info.get().get_blob_info());
                        }
                        catch(...)
                        {
                          callback(Failable<BlobInfo>(std::current_exception()));
                        }
                      });
  }
}

void
ResourceManager::request_file_info(const std::string& filename,
                                   const std::function<void (Failable<FileInfo>)>& callback)
{
  m_database.request_file_info
    (filename,
     [this, filename, callback](const boost::optional<FileInfo>& db_file_info)
     {
      if (db_file_info)
      {
        callback(*db_file_info);
      }
      else
      {
        m_generator.request_file_info
          (filename,
           [this, callback](const Failable<FileInfo>& file_info)
           {
            if (file_info.is_initialized())
            {
              m_database.store_file_info(file_info.get(), callback);
            }
            else
            {
              callback(file_info);
            }
          });
      }
    });
}

void
ResourceManager::request_url_info(const std::string& url,
                                  const std::function<void (Failable<URLInfo>)>& callback)
{
  m_database.request_url_info
    (url,
     [this, url, callback](const boost::optional<URLInfo>& db_url_info)
     {
       if (db_url_info)
       {
         callback(*db_url_info);
       }
       else
       {
         m_download_mgr.request_get
           (url,
            [this, url, callback](const DownloadResult& result)
            {
              if (result.success())
              {
                m_database.store_url_info
                  (URLInfo(url, result.get_mtime(), result.get_content_type(),
                           BlobInfo::from_blob(result.get_blob())),
                   [callback](const Failable<URLInfo>& url_info)
                   {
                     callback(url_info);
                   });
              }
              else
              {
                callback(Failable<URLInfo>::from_exception(std::runtime_error("request URLInfo failed")));
              }
            });
       };
     });
}

void
ResourceManager::request_resource_info(const ResourceLocator& locator, const BlobInfo& blob,
                                       const std::function<void (Failable<ResourceInfo>)>& callback)
{
  m_database.request_resource_info
    (locator, blob,
     [this, locator, blob, callback](const boost::optional<ResourceInfo>& reply)
     {
       if (reply)
       {
         callback(*reply);
       }
       else
       {
         m_generator.request_resource_processing
           (locator, std::make_shared<ResourceGeneratorCallbacks>());

         callback(Failable<ResourceInfo>::from_exception(
                    std::runtime_error("ResourceManager::request_resource_info(): not implemented")));
       }
     });
}

void
ResourceManager::request_resource_info(const ResourceLocator& locator,
                                       const std::function<void (const Failable<ResourceInfo>&)>& callback)
{
  if (locator.get_url().get_scheme() == "file")
  {
    request_file_info
      (locator.get_url().get_path(),
       [this, locator, callback](const Failable<FileInfo>& data)
       {
         try
         {
           const FileInfo& file_info = data.get();
           request_resource_info(locator, file_info.get_blob_info(), callback);
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
}

void
ResourceManager::request_image_info(const ResourceInfo& resource,
                                    const std::function<void (const Failable<ImageInfo>&)>& callback)
{
  m_database.request_image_info
    (resource,
     [this, resource, callback](const boost::optional<ImageInfo>& image_info)
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
    [this](const boost::optional<TileEntry>& tile_entry)
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
        [this](const boost::optional<TileEntry>& tile_entry)
        {
          callback(TileInfo(tile_entry));
        });
    });
}
#endif

/* EOF */

