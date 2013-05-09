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

#include "archive/archive_manager.hpp"
#include "archive/extraction.hpp"
#include "galapix/tile.hpp"
#include "generator/archive_data.hpp"
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

Generator::Generator(BlobManager& blob_mgr, ArchiveManager& archive_mgr) :
  m_blob_mgr(blob_mgr),
  m_archive_mgr(archive_mgr),
  m_pool(4)
{
}

Generator::~Generator()
{
  // TODO: make sure no requests are still going before shutting down
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
     [this, locator, callbacks](const Failable<BlobAccessorPtr>& result)
     {
       try
       {
         BlobAccessorPtr blob = result.get();
         m_pool.schedule
           ([this, locator, blob, callbacks]
            {
              process_resource(blob, callbacks);
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
Generator::process_resource(const BlobAccessorPtr& blob_accessor, GeneratorCallbacksPtr callbacks)
{
  // FIXME: 
  // 1) use BlobAccessor instead
  // 2) pass BlobAccessor to SurfaceFactory
  //    - const SoftwareSurfaceLoader* find_loader_by_filename(const std::string& filename) const;
  //    - const SoftwareSurfaceLoader* find_loader_by_magic(const std::string& data) const;
  // 3) add magic to detect archives
  // 4) return ResourceInfo

  // generate BlobInfo from the Blob
  BlobInfo blob_info = BlobInfo::from_blob(blob_accessor);
  callbacks->on_blob_info(blob_info);

  if (process_image_resource(blob_accessor, blob_info, callbacks))
  {
    callbacks->on_success(ResourceStatus::Success);
  }
  else if (process_archive_resource(blob_accessor, callbacks))
  {
    callbacks->on_success(ResourceStatus::Success);
  }
  else
  {
    callbacks->on_error(ResourceStatus::UnknownHandler, "Generator: unknown resource type");
  }
}

bool
Generator::process_archive_resource(const BlobAccessorPtr& blob_accessor, GeneratorCallbacksPtr callbacks)
{
  try
  {
    ExtractionPtr extraction = m_archive_mgr.get_extraction(blob_accessor->get_stdio_name());
  
    std::vector<ArchiveFileInfo> files;
    for(const auto& filename : extraction->get_filenames())
    {
      BlobAccessorPtr child_blob = std::make_shared<BlobAccessor>(extraction->get_file(filename));
      BlobInfo child_blob_info = child_blob->get_blob_info();
      files.emplace_back(filename, child_blob_info);

      ResourceLocator child_locator; // TODO: fill this with stuff
      GeneratorCallbacksPtr child_callbacks = callbacks->on_child_resource(child_locator);
      process_resource(child_blob, child_callbacks);
    }

    ArchiveInfo archive_info(std::move(files), boost::optional<std::string>());
    callbacks->on_archive_data(archive_info);
    
    return true;
  }
  catch(const std::exception& err)
  {
    callbacks->on_error(ResourceStatus::HandlerError, err.what());
  }

  return false;
}

bool
Generator::process_image_resource(const BlobAccessorPtr& blob_accessor, const BlobInfo& blob_info, GeneratorCallbacksPtr callbacks)
{
  // generate ImageData if it is an image
  const SoftwareSurfaceLoader* loader = nullptr;
  if (blob_accessor->has_stdio_name())
  {
    loader = SoftwareSurfaceFactory::current().find_loader_by_magic(Filesystem::get_magic(blob_accessor->get_stdio_name()));
  }
  else
  {
    loader = SoftwareSurfaceFactory::current().find_loader_by_magic(blob_accessor->get_blob());
  }

  if (!loader)
  {
    return false;
  }
  else
  {
    ResourceName resource_name(blob_info, ResourceHandler("image", loader->get_name()));

    // mark as in progress
    callbacks->on_resource_info(ResourceInfo(resource_name, ResourceStatus::InProgress));

    try
    {
      SoftwareSurfacePtr surface;

      if (blob_accessor->has_stdio_name())
      {
        surface = loader->from_file(blob_accessor->get_stdio_name());
      }
      else
      {
        surface = loader->from_mem(blob_accessor->get_blob()->get_data(), blob_accessor->get_blob()->size());
      }

      //callbacks->on_image_info(ImageInfo(RowId(), surface->get_width(), surface->get_height()));
      process_image_tiling(surface, callbacks);
      
      // mark as success
      callbacks->on_resource_info(ResourceInfo(resource_name, ResourceStatus::Success));
      return true;
    }
    catch(const std::exception& err)
    {
      callbacks->on_error(ResourceStatus::AccessError, err.what());
      return false;
    }
  }
}

void
Generator::process_image_tiling(SoftwareSurfacePtr surface, GeneratorCallbacksPtr callbacks)
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
}

/* EOF */
