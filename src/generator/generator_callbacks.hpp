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

#ifndef HEADER_GALAPIX_GENERATOR_GENERATOR_CALLBACKS_HPP
#define HEADER_GALAPIX_GENERATOR_GENERATOR_CALLBACKS_HPP

#include <memory>

#include "resource/resource_status.hpp"

class ArchiveData;
class ArchiveFileInfo;
class ArchiveInfo;
class BlobInfo;
class GeneratorCallbacks;
class ImageData;
class ImageInfo;
class ResourceInfo;
class ResourceLocator;
class ResourceName;
class VideoData;
class VideoInfo;

using GeneratorCallbacksPtr = std::shared_ptr<GeneratorCallbacks>;

/** The GeneratorCallbacks class receives all the data that the
    Generator produces while processing a resource. The data is send
    incrementally whenever it is ready. */
class GeneratorCallbacks
{
public:
  virtual ~GeneratorCallbacks() {}

  /** If the resource contains a child resource, such as an archive
      containing a file, then this callback gets called. The user
      needs to return a GeneratorCallbacksPtr that is applied to the
      child resource */
  virtual GeneratorCallbacksPtr on_child_resource(ResourceLocator const& locator) = 0;

  /** Once the file is opened and the SHA1 calculated, the BlobInfo is returned */
  virtual void on_blob_info (BlobInfo const& blob_info) = 0;

  /** This function gets called as soon as the generator has
      determined the resource type of the given locator. */
  virtual void on_resource_name (ResourceName const& resource_name) = 0;

  /** Called only on archives and contains info on the files within an
      archive */
  virtual void on_archive_data (ArchiveInfo const& archive_info) = 0;

  /** Called only for images and contains information on width/height
      as well as the actual image tiles */
  virtual void on_image_data (ImageData const& image_data) = 0;

  /** Called when the status changes, the status indicates if
      generation was done completely or incrementally. */
  virtual void on_status(ResourceStatus status) = 0;

  /** Called when an error occured, no further processing will be done
      after this point. */
  virtual void on_error(ResourceStatus status, std::string const& err) = 0;
};

#endif

/* EOF */
