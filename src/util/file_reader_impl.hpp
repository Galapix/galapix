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

#ifndef HEADER_GALAPIX_UTIL_FILE_READER_IMPL_HPP
#define HEADER_GALAPIX_UTIL_FILE_READER_IMPL_HPP

#include <vector>
#include <string>

class Size;
class RGBA;
class Vector2i;

class FileReader;

class FileReaderImpl
{
public:
  FileReaderImpl() {}
  virtual ~FileReaderImpl() {}

  virtual std::string get_name()                           const =0;
  virtual bool read_int   (const char* name, int&)         const =0;
  virtual bool read_float (const char* name, float&)       const =0;
  virtual bool read_bool  (const char* name, bool&)        const =0;
  virtual bool read_string(const char* name, std::string&) const =0;
  virtual bool read_size  (const char* name, Size&)        const =0;
  virtual bool read_vector2i(const char* name, Vector2i&)  const =0;
  virtual bool read_vector2f(const char* name, Vector2f&)  const =0;
  virtual bool read_rect  (const char* name, Rect&)        const =0;
  virtual bool read_rgba  (const char* name, RGBA&)        const =0;
  virtual bool read_section(const char* name, FileReader&) const =0;
  virtual std::vector<FileReader> get_sections() const =0;
  virtual std::vector<std::string> get_section_names() const =0;
};

#endif

/* EOF */
