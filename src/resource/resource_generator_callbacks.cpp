/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#include "resource/resource_generator_callbacks.hpp"

ResourceGeneratorCallbacks::ResourceGeneratorCallbacks()
{
}

ResourceGeneratorCallbacks::~ResourceGeneratorCallbacks()
{
}

GeneratorCallbacksPtr
ResourceGeneratorCallbacks::on_child_resource(const ResourceLocator& locator)
{
 return GeneratorCallbacksPtr();
}

void
ResourceGeneratorCallbacks::on_blob_info(const BlobInfo & blob_info)
{
}

void
ResourceGeneratorCallbacks::on_resource_info(const ResourceInfo & resource_info)
{
}

void
ResourceGeneratorCallbacks::on_archive_data(const ArchiveInfo & archive_info)
{
}

void
ResourceGeneratorCallbacks::on_image_data(const ImageData & image_data)
{
}

void
ResourceGeneratorCallbacks::on_success(ResourceStatus status)
{
}

void
ResourceGeneratorCallbacks::on_error(ResourceStatus status, const std::string& err)
{
}

/* EOF */
