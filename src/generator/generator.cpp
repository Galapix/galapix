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

#include "generator/generator.hpp"

#include "galapix/tile.hpp"
#include "generator/image_data.hpp"
#include "jobs/tile_generator.hpp"
#include "resource/blob_manager.hpp"
#include "resource/image_info.hpp"
#include "resource/resource_cache_state.hpp"
#include "resource/resource_info.hpp"
#include "resource/resource_status.hpp"
#include "util/log.hpp"
#include "util/software_surface_factory.hpp"
#include "util/software_surface_loader.hpp"

Generator::Generator(BlobManager& blob_mgr) :
  m_blob_mgr(blob_mgr),
  m_pool(4)
{
}

Generator::~Generator()
{
}

void
Generator::request_file_info(const std::string& path, 
                             const std::function<void (const Failable<FileInfo>&)>& callback)
{
  log_info(path);
  
  m_pool.schedule
    ([path, callback]{
      try 
      {
        log_debug("trying to get FileInfo");

        Failable<FileInfo> result;
        result.reset(FileInfo::from_file(path));
        callback(result);

        log_debug("trying to get FileInfo: success");
      }
      catch(...)
      {
        log_debug("trying to get FileInfo: failed");
        Failable<FileInfo> result;
        result.set_exception(std::current_exception());
        callback(result);
      }
    });
}

void
Generator::request_resource_processing(const ResourceLocator& locator,
                                       GeneratorCallbacksPtr callbacks)
{
  m_blob_mgr.request_blob
    (locator,
     [this, locator, callbacks](const Failable<BlobPtr>& result)
     {
       try
       {
         BlobPtr blob = result.get();
         m_pool.schedule
           ([this, locator, blob, callbacks]
            {
              process_resource(locator, blob, callbacks);
            });
       }
       catch(const std::exception& err)
       {
         callbacks->on_error(ResourceStatus::AccessError, err.what());
       }
       catch(...)
       {
         callbacks->on_error(ResourceStatus::AccessError, "unknown exception");
       }
     });
}

void
Generator::process_resource(const ResourceLocator& locator, const BlobPtr& blob,
                            GeneratorCallbacksPtr callbacks)
{
  // FIXME: 
  // 1) use BlobAccessor instead
  // 2) pass BlobAccessor to SurfaceFactory
  //    - const SoftwareSurfaceLoader* find_loader_by_filename(const std::string& filename) const;
  //    - const SoftwareSurfaceLoader* find_loader_by_magic(const std::string& data) const;
  // 3) add magic to detect archives
  // 4) return ResourceInfo

  // generate BlobInfo from the Blob
  BlobInfo blob_info = BlobInfo::from_blob(blob);
  callbacks->on_blob_info(blob_info);

  // generate ImageData if it is an image
  const SoftwareSurfaceLoader* loader = SoftwareSurfaceFactory::current().find_loader_by_magic(blob);
  if (loader)
  {
    ResourceName resource_name(blob_info, ResourceHandler("image", loader->get_name()));

    // mark as in progress
    callbacks->on_resource_info(ResourceInfo(resource_name, ResourceStatus::InProgress));

    try
    {
      SoftwareSurfacePtr surface = loader->from_mem(blob->get_data(), blob->size());
      //callbacks->on_image_info(ImageInfo(RowId(), surface->get_width(), surface->get_height()));
      process_image_resource(surface, callbacks);
      
      // mark as success
      callbacks->on_resource_info(ResourceInfo(resource_name, ResourceStatus::Success));
    }
    catch(const std::exception& err)
    {
      callbacks->on_error(ResourceStatus::AccessError, err.what());
    }
  }
  else if (false /* ArchiveManager::is_archive(blob)) */)
  {
    // find archive type and stuff
    callbacks->on_error(ResourceStatus::UnknownHandler, "Generator: not implemented");
  }
  else
  {
    callbacks->on_error(ResourceStatus::UnknownHandler, "Generator: unknown resource type");
  }
}

void
Generator::process_image_resource(SoftwareSurfacePtr surface, GeneratorCallbacksPtr callbacks)
{
  std::vector<Tile> tiles;

  int min_scale = 0;
  int max_scale = 8; log_error("FIXME: incorrect max scale");

  TileGenerator::generate(surface, min_scale, max_scale,
                          [&tiles](int x, int y, int scale, SoftwareSurfacePtr tile)
                          {
                            tiles.emplace_back(Tile(scale, Vector2i(x, y), tile));
                          });

  // build ImageData
  ImageData image_data(ImageInfo(RowId(), surface->get_width(), surface->get_height()), 
                       std::move(tiles));
  callbacks->on_image_data(image_data);
  callbacks->on_success(ResourceStatus::Success);
}

/* EOF */
