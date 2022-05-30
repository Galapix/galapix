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

#ifndef HEADER_GALAPIX_GENERATOR_GENERATOR_HPP
#define HEADER_GALAPIX_GENERATOR_GENERATOR_HPP

#include <map>
#include <string>
#include <vector>

#include <arxp/fwd.hpp>
#include <surf/software_surface.hpp>

#include "resource/file_info.hpp"
#include "util/failable.hpp"
#include "util/thread_pool.hpp"
#include "generator/generator_callbacks.hpp"

class BlobInfo;
class BlobManager;
class Extraction;
class ImageInfo;
class ResourceInfo;
class ResourceLocator;

class Generator
{
public:
  Generator(BlobManager& blob_mgr, arxp::ArchiveManager& archive_mgr);
  ~Generator();

  void request_file_info(std::string const& path,
                         const std::function<void (Failable<FileInfo> const&)>& callback);

  void request_resource_processing(ResourceLocator const& locator,
                                   GeneratorCallbacksPtr const& callbacks);

private:
  void process_resource(ResourceLocator const& locator, BlobAccessorPtr const& blob_accessor, GeneratorCallbacksPtr const& callbacks);
  bool process_image_resource(ResourceLocator const& locator, BlobAccessorPtr const& blob_accessor, BlobInfo const& blob_info,
                              GeneratorCallbacksPtr const& callbacks);
  bool process_archive_resource(ResourceLocator const& locator, BlobAccessorPtr const& blob_accessor,
                                GeneratorCallbacksPtr const& callbacks);
  void process_image_tiling(surf::SoftwareSurface const&  surface, GeneratorCallbacksPtr const& callbacks);

private:
  BlobManager& m_blob_mgr;
  arxp::ArchiveManager& m_archive_mgr;
  ThreadPool m_pool;

private:
  Generator(Generator const&) = delete;
  Generator& operator=(Generator const&) = delete;
};

#endif

/* EOF */
