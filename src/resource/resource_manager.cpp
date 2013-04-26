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
ResourceManager::request_blob(const ResourceLocator& locator, 
                              const std::function<void (Failable<BlobPtr>)>& callback)
{
  if (locator.get_handler().empty())
  {
    ResourceURL url = locator.get_url();
    if (url.get_scheme() == "file")
    {
      std::async(std::launch::async, [=]{
          try 
          {
            BlobPtr blob = Blob::from_file(url.get_path());
            Failable<BlobPtr> failable(blob);
            callback(blob);
          }
          catch(const std::exception& err)
          {
            Failable<BlobPtr> failable;
            failable.set_exception(std::make_exception_ptr(err));
            callback(failable);
          }
        });
    }
    else if (url.get_scheme() == "http"  ||
             url.get_scheme() == "https" ||
             url.get_scheme() == "ftp")
    {
      m_download_mgr.request_get(url.str(), [=](const DownloadResult& result) {
          if (result.success())
          {
            callback(Failable<BlobPtr>(result.get_blob()));
          }
          else
          {
            Failable<BlobPtr> failable;
            failable.set_exception(std::make_exception_ptr(std::runtime_error(format("%s: error: invalid response code: %d", 
                                                                                     url.str(), result.get_response_code()))));
            callback(failable);
          }
        });
    }
    else
    {
      Failable<BlobPtr> failable;
      failable.set_exception(std::make_exception_ptr(std::runtime_error(format("%s: error: unsupported URL scheme: %s", 
                                                                               locator.str(), url.get_scheme()))));
      callback(failable);
    }
  }
  else
  {
    
  }
}

void
ResourceManager::request_sha1(const ResourceLocator& locator, 
                              const std::function<void (Failable<SHA1>)>& callback)
{
  ResourceLocator blob_locator = locator.get_blob_locator();
  if (blob_locator.get_url().is_remote())
  {
    request_url_info(locator.get_url().get_path(),
                     [callback](const Failable<URLInfo>& url_info)
                     {
                       try
                       {
                         callback(url_info.get().get_sha1());
                       }
                       catch(...)
                       {
                         callback(Failable<SHA1>(std::current_exception()));
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
                          callback(file_info.get().get_sha1());
                        }
                        catch(...)
                        {
                          callback(Failable<SHA1>(std::current_exception()));
                        }
                      });    
  }
}

void
ResourceManager::request_file_info(const std::string& filename, 
                                   const std::function<void (Failable<FileInfo>)>& callback)
{
  m_database.request_file_info(filename, [this, filename, callback](const boost::optional<FileInfo>& db_file_info){
      if (db_file_info)
      {
        callback(*db_file_info);
      }
      else
      {
        m_generator.request_file_info(filename, [this, callback](const Failable<FileInfo>& file_info){
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
#if 0
  m_database.request_url_info
    (url, 
     [](const boost::optional<URLInfo>& url_info)
     {
       if (url_info)
       {
         callback(url_info);
       }
       else
       {
         m_download_mgr.request_get
           (url,
            [](const DownloadResult& result)
            {
              if (result.success())
              {
                URLInfo info(result.get_mtime(), result.get_content_type());
                callback(url_info);
              }
              else
              {
                Failable<ResourceInfo> failable;
                failable.set_exception(std::make_exception_ptr(std::runtime_error("request URLInfo failed")));
                callback(failable);
              }
            }
       }
     });
#endif
}

void
ResourceManager::request_resource_info(const ResourceLocator& locator, const SHA1& sha1,
                                       const std::function<void (Failable<ResourceInfo>)>& callback)
{
  Failable<ResourceInfo> result;
  result.set_exception(std::make_exception_ptr(std::runtime_error("ResourceInfo request not yet implemented")));
  callback(result);    

  /*
   m_database.request_resource_info
    (locator, sha1,
     [this, callback](const boost::optional<ResourceInfo>& resource_info) 
     {
       if (resource_info)
       {
         callback(*resource_info);
       }
       else
       {
         log_error("not implemented");
         m_generator.request_resource_info(??? [this, callback](const Failable<ResourceInfo>&){
          
           });
        }
     });
  */
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
           request_resource_info(locator, file_info.get_sha1(), callback);
         }
         catch(...)
         {
           Failable<ResourceInfo> result;
           result.set_exception(std::current_exception());
           callback(result);
         }
       });
  }
  else if (locator.get_url().is_remote())
  {
    Failable<ResourceInfo> result;
    result.set_exception(std::make_exception_ptr(std::runtime_error("remote URLs not yet implemented")));
    callback(result);    
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
     [this, resource, callback](const boost::optional<ImageInfo> image_info)
     {
       if (image_info)
       {
         callback(*image_info);
       }
       else
       {
        m_generator.request_image_info
          (resource, 
           // full generation or incremental generation
           [this, callback](const Failable<ImageInfo>& result)
           {
             
           });
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
    [this](const boost::optional<TileEntry> tile_entry)
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
