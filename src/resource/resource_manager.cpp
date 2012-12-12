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

#include "resource/archive_info.hpp"
#include "galapix/database_thread.hpp"
#include "resource/image_info.hpp"
#include "resource/tile_info.hpp"

ResourceManager::ResourceManager(DatabaseThread& database, Generator& generator) :
  m_database(database),
  m_generator(generator)
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

/* EOF */
