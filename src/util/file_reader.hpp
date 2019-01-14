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

#ifndef HEADER_GALAPIX_UTIL_FILE_READER_HPP
#define HEADER_GALAPIX_UTIL_FILE_READER_HPP

#include <vector>
#include <memory>

#include "math/vector2i.hpp"
#include "math/vector2f.hpp"

class RGBA;
class Size;
class Rect;

class FileReaderImpl;
class URL;

/** Interface to read name/value pairs out of some kind of file or
    structure */
class FileReader
{
public:
  FileReader(std::shared_ptr<FileReaderImpl> const& impl_);
  FileReader();
  virtual ~FileReader() {}

  /** Name of the current section, ie. in the case of
      <groundpiece><pos>...</groundpiece> it would be 'groundpiece' */
  std::string get_name() const;

  bool read_int   (const char* name, int&)           const;
  bool read_float (const char* name, float&)         const;
  bool read_bool  (const char* name, bool&)          const;
  bool read_string(const char* name, std::string&)   const;
  bool read_url   (const char* name, URL&)           const;
  bool read_vector2i(const char* name, Vector2i&)    const;
  bool read_vector2f(const char* name, Vector2f&)    const;
  bool read_rect(const char* name, Rect&)    const;
  bool read_size  (const char* name, Size&)          const;
  bool read_rgba (const char* name, RGBA&)         const;
  bool read_section(const char* name, FileReader&)   const;
  FileReader read_section(const char* name)   const;

  template<class E, class T>
  bool read_enum  (const char* name, E& value, T enum2string) const
  {
    std::string str;
    if (read_string(name, str))
      {
        value = enum2string(str);
        return true;
      }

    return false;
  }

  std::vector<std::string> get_section_names() const;
  std::vector<FileReader>  get_sections() const;
  int  get_num_sections() const;

  static FileReader parse(const std::string& filename);

private:
  std::shared_ptr<FileReaderImpl> impl;
};

#endif

/* EOF */
