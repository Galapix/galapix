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

class ResourceInfo;
class ArchiveInfo;
class ArchiveFileInfo;
class ImageInfo;
class ImageData;
class VideoInfo;
class VideoData;

class GeneratorCallbacks
{
private:
public:
  virtual ~GeneratorCallbacks() {}

  virtual void on_resource_info    (const ResourceInfo    & resource_info)     = 0;

  virtual void on_archive_info     (const ArchiveInfo     & archive_info)      = 0;
  virtual void on_archive_file_info(const ArchiveFileInfo & archive_file_info) = 0;

  virtual void on_image_info       (const ImageInfo       & image_info)        = 0;
  virtual void on_image_data       (const ImageData       & image_data)        = 0;

  virtual void on_video_info       (const VideoInfo       & video_info)        = 0;
  virtual void on_video_data       (const VideoData       & video_data)        = 0;
};

typedef std::shared_ptr<GeneratorCallbacks> GeneratorCallbacksPtr;

#endif

/* EOF */
