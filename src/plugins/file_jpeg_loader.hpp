/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_WINDSTILLE_FILE_JPEG_LOADER_HPP
#define HEADER_WINDSTILLE_FILE_JPEG_LOADER_HPP

#include "plugins/jpeg_loader.hpp"

#include <sstream>
#include <stdio.h>

class FileJPEGLoader : public JPEGLoader
{
private:
  std::string m_filename;
  FILE* m_in;

public:
  FileJPEGLoader(const std::string& filename);
  ~FileJPEGLoader();

private:
  FileJPEGLoader(const FileJPEGLoader&);
  FileJPEGLoader& operator=(const FileJPEGLoader&);
};

#endif

/* EOF */
