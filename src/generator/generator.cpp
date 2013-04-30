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

#include "resource/blob_manager.hpp"
#include "resource/image_info.hpp"
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
Generator::request_resource_info(const ResourceLocator& locator, const BlobInfo& blob_info,
                                 const std::function<void (Failable<ResourceInfo>)>& callback)
{
  m_blob_mgr.request_blob
    (locator,
     [this, locator, blob_info, callback](const Failable<BlobPtr>& result)
     {
       try
       {
         BlobPtr blob = result.get();
         m_pool.schedule
           ([this, locator, blob, blob_info, callback]
            {
              try
              {
                //ResourceInfo info = generate_resource_info(locator, blob, blob_info);
                //callback(info);
              }
              catch(...)
              {
                callback(Failable<ResourceInfo>(std::current_exception()));
              }
            });
       }
       catch(...)
       {
         callback(Failable<ResourceInfo>(std::current_exception()));
       }
     });
}

void
Generator::request_resource_processing(const ResourceLocator& locator,
                                       GeneratorCallbacksPtr callbacks)
{
  
}

void
Generator::request_resource_processing(const ResourceLocator& locator, const BlobInfo& blob_info,
                                       GeneratorCallbacksPtr callbacks)
{
#if 0
  m_blob_mgr.request_blob
    (locator,
     [this, locator, blob_info, callbacks](const Failable<BlobPtr>& result)
     {
       try
       {
         BlobPtr blob = result.get();
         m_pool.schedule
           ([this, locator, blob, blob_info, callbacks]
            {
              try
              {
                ResourceInfo info = generate_resource_info(locator, blob, blob_info);
                callback(info);
              }
              catch(...)
              {
                callback(Failable<ResourceInfo>(std::current_exception()));
              }
            });
       }
       catch(...)
       {
         callback(Failable<ResourceInfo>(std::current_exception()));
       }
     });
#endif
}

void
Generator::process_resource(const ResourceLocator& locator, const BlobPtr& blob, const BlobInfo& blob_info,
                            GeneratorCallbacksPtr callbacks)
{
  // FIXME: 
  // 1) use BlobAccessor instead
  // 2) pass BlobAccessor to SurfaceFactory
  //    - const SoftwareSurfaceLoader* find_loader_by_filename(const std::string& filename) const;
  //    - const SoftwareSurfaceLoader* find_loader_by_magic(const std::string& data) const;
  // 3) add magic to detect archives
  // 4) return ResourceInfo
  const SoftwareSurfaceLoader* loader = SoftwareSurfaceFactory::current().find_loader_by_magic(blob);
  if (loader)
  {
    ResourceHandler handler("image", loader->get_name(), std::string());
    ResourceInfo resource_info(RowId(), 
                               ResourceName(blob_info, handler), 
                               ResourceStatus::InProgress);
    callbacks->on_resource_info(resource_info);
  }
  else
  {
    throw std::runtime_error("Generator: unknown image type");
  }
}

void
Generator::request_image_info(const ResourceInfo& resource,
                              const std::function<void (const Failable<ImageInfo>&)>& callback)
{
#if 0
  m_pool.schedule
    ([]{
      try 
      {
        SoftwareSurfacePtr surface;
        Size size;
        int min_scale;
        int max_scale;
    
        // FIXME: JPEG::filename_is_jpeg() is ugly
        if (!m_url.is_remote() && JPEG::filename_is_jpeg(m_url.str()))
        {
          BlobPtr blob;

          if (m_url.has_stdio_name())
          {
            size = JPEG::get_size(m_url.get_stdio_name());
          }
          else
          {
            blob = m_url.get_blob();
            size = JPEG::get_size(blob->get_data(), blob->size());
          }

          // FIXME: On http:// transfer mtime and size must be got from the transfer itself, not afterwards
          file_entry = FileEntry(RowId(), m_url, m_url.get_size(), m_url.get_mtime(), FileEntry::kUnknownHandler);

          // 2^3 is the highest scale JPEG supports, so we limit the
          // min_scale to that
          min_scale = Math::min(min_scale, 3);

          // FIXME: recalc min_scale from jpeg scale
          if (!blob)
          {
            surface = JPEG::load_from_file(m_url.get_stdio_name(), Math::pow2(min_scale));
          }
          else
          {
            surface = JPEG::load_from_mem(blob->get_data(), blob->size(), Math::pow2(min_scale));
          }
        }
        else
        {
          // FIXME: On http:// transfer mtime and size must be got from the transfer itself, not afterwards
          surface = SoftwareSurfaceFactory::current().from_url(m_url);
          size = surface->get_size();
          file_entry = FileEntry(RowId(), m_url, m_url.get_size(), m_url.get_mtime(), FileEntry::kImageHandler);
          min_scale = 0;
          max_scale = file_entry.get_thumbnail_scale();
        }

        m_sig_file_callback(file_entry);
    
        TileGenerator::cut_into_tiles(surface, size, min_scale, max_scale, 
                                      [this](const Tile& tile){
                                        m_sig_tile_callback(tile);
                                      });
      }
      catch(const std::exception& err)
      {
        log_error("Error while processing " << m_url);
        log_error("  Exception: " << err.what());
      }
    });
#endif
}

/* EOF */
