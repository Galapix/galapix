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

#ifndef HEADER_GALAPIX_GENERATOR_GENERATOR_CALLBACKS_HPP
#define HEADER_GALAPIX_GENERATOR_GENERATOR_CALLBACKS_HPP

#include "resource/resource_status.hpp"

class ArchiveData;
class ArchiveFileInfo;
class ArchiveInfo;
class GeneratorCallbacks;
class ImageData;
class ImageInfo;
class ResourceInfo;
class ResourceLocator;
class VideoData;
class VideoInfo;

typedef std::shared_ptr<GeneratorCallbacks> GeneratorCallbacksPtr;

class GeneratorCallbacks
{
private:
public:
  virtual ~GeneratorCallbacks() {}

  virtual GeneratorCallbacksPtr on_child_resource(const ResourceLocator& locator) = 0;

  virtual void on_blob_info        (const BlobInfo     & blob_info)     = 0;
  virtual void on_resource_info    (const ResourceInfo & resource_info) = 0;

  virtual void on_archive_data     (const ArchiveInfo & archive_info)   = 0;
  virtual void on_image_data       (const ImageData   & image_data)     = 0;

  virtual void on_success(ResourceStatus status) = 0;
  virtual void on_error(ResourceStatus status, const std::string& err) = 0;
};

#endif

/* EOF */
