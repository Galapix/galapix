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

ResourceManager::ResourceManager(DatabaseThread& database) :
  m_quit(false),
  m_database(database),
  m_queue()
{
}

void
ResourceManager::request_tile_info(const ImageInfo& image, int scale, int x, int y,
                                   const std::function<void (const TileInfo&)>& callback)
{
  m_queue.wait_and_push(
    [=]
    {
      /*
        m_database.request_tile(image.get_rowid(), scale, x, y, 
        [](const TileEntry& tile_entry){ 
        //callback(); 
        });
      */
    });
}

void
ResourceManager::request_image_info(const ResourceInfo& resource,
                                    const std::function<void (const ImageInfo&)>& callback)
{
}

void
ResourceManager::request_archive_info(const ResourceInfo& resource,
                                      const std::function<void (const ArchiveInfo&)>& callback)
{
}

void
ResourceManager::request_resource_info(const ResourceLocator& locator,
                                       const std::function<void (const ResourceInfo&)>& callback)
{
  /*
  m_database.request_file_entry(locator,
                                [](const boost::optional<FileEntry>& file_entry)
                                {
                                  if (file_entry)
                                  {
                                    [=]
                                    {
                                      m_database.request_resource(image.get_rowid(), scale, x, y, 
                                                                  [](const ResourceEntry& resource_entry)
                                                                  { 
                                    
                                                                  });
                                    } 
                                  }
                                  else
                                  {
                                        
                                  }
                                });
  */
}

void
ResourceManager::run()
{
  while(!m_quit)
  {
    std::function<void()> func;
    while(m_queue.try_pop(func))
    {
      func();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void
ResourceManager::stop_thread()
{
  m_quit = true;
  m_queue.wakeup();
}

/* EOF */
