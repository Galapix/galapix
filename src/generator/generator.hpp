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

#ifndef HEADER_GALAPIX_GENERATOR_GENERATOR_HPP
#define HEADER_GALAPIX_GENERATOR_GENERATOR_HPP

#include "resource/file_info.hpp"
#include "util/failable.hpp"
#include "util/thread_pool.hpp"

class ImageInfo;
class ResourceInfo;

class Generator
{
private:
  ThreadPool m_pool;
  
public:
  Generator();
  ~Generator();

  void request_file_info(const std::string& path, 
                         const std::function<void (const Failable<FileInfo>&)>& callback);

  void request_image_info(const ResourceInfo& resource,
                          const std::function<void (const Failable<ImageInfo>&)>& callback);

private:
  Generator(const Generator&);
  Generator& operator=(const Generator&);
};

#endif

/* EOF */
