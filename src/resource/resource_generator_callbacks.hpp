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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_GENERATOR_CALLBACKS_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_GENERATOR_CALLBACKS_HPP

#include "generator/generator_callbacks.hpp"
#include "resource/resource_locator.hpp"

class ResourceGeneratorCallbacks : public GeneratorCallbacks
{
public:
  ResourceGeneratorCallbacks(const ResourceLocator& locator);
  ~ResourceGeneratorCallbacks() override;

  GeneratorCallbacksPtr on_child_resource(const ResourceLocator& locator) override;

  void on_blob_info(const BlobInfo& blob_info) override;
  void on_resource_name(const ResourceName& resource_name) override;

  void on_archive_data(const ArchiveInfo& archive_info) override;
  void on_image_data(const ImageData& image_data) override;

  void on_status(ResourceStatus status) override;
  void on_error(ResourceStatus status, const std::string& err) override;

private:
  ResourceLocator m_locator;
};

#endif

/* EOF */
