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
#include "network/download_manager.hpp"
#include "network/download_result.hpp"
#include "resource/archive_info.hpp"
#include "resource/image_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_url.hpp"
#include "resource/tile_info.hpp"
#include "util/format.hpp"

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
ResourceManager::request_info(const ResourceLocator& locator, 
                              const std::function<void (Failable<ResourceInfo>)>& callback)
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
    else if (url.get_scheme() == "http" ||
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
ResourceManager::request_resource_info(const ResourceLocator& locator,
                                       const std::function<void (const Failable<ResourceInfo>&)>& callback)
{
#if 0
  auto file_entry_callback = [](const boost::optional<FileEntry>& file_entry)
    {
      if (!file_entry)
      {
        m_generator.request_file_entry();
      }
      else
      {
        m_database.request_resource(image.get_rowid(), scale, x, y, 
                                  [](const ResourceEntry& resource_entry));
      }
    };

  m_database.request_file_entry(locator, file_entry_callback);
#endif
}

void
ResourceManager::request_image_info(const ResourceInfo& resource,
                                    const std::function<void (const Failable<ImageInfo>&)>& callback)
{
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
