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

#include "resource/resource_generator_callbacks.hpp"

#include <logmich/log.hpp>

#include "generator/image_data.hpp"
#include "resource/archive_info.hpp"
#include "resource/blob_info.hpp"
#include "resource/resource_info.hpp"
#include "resource/resource_locator.hpp"

namespace galapix {

ResourceGeneratorCallbacks::ResourceGeneratorCallbacks(ResourceLocator const& locator) :
  m_locator(locator)
{
}

ResourceGeneratorCallbacks::~ResourceGeneratorCallbacks()
{
}

GeneratorCallbacksPtr
ResourceGeneratorCallbacks::on_child_resource(ResourceLocator const& locator)
{
  log_info(locator.str());
  return std::make_shared<ResourceGeneratorCallbacks>(locator);
}

void
ResourceGeneratorCallbacks::on_blob_info(BlobInfo const& blob_info)
{
  log_info(blob_info.get_sha1().str());
}

void
ResourceGeneratorCallbacks::on_resource_name(ResourceName const& resource_name)
{
  log_info(resource_name.str());
}

void
ResourceGeneratorCallbacks::on_archive_data(ArchiveInfo const& archive_info)
{
  log_info("{}", archive_info.get_files().size());
}

void
ResourceGeneratorCallbacks::on_image_data(ImageData const& image_data)
{
  log_info("{} {}x{}",
           image_data.get_image_tiles().size(),
           image_data.get_image_info().get_width(),
           image_data.get_image_info().get_height());
}

void
ResourceGeneratorCallbacks::on_status(ResourceStatus status)
{
  log_info(to_string(status));
}

void
ResourceGeneratorCallbacks::on_error(ResourceStatus status, std::string const& err)
{
  log_info(err);
}

} // namespace galapix

/* EOF */
