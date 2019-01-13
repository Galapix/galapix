/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_UTIL_FILE_WRITER_HPP
#define HEADER_GALAPIX_UTIL_FILE_WRITER_HPP

#include <string>

class Size;

/** Interface to write out name/value pairs out of some kind of file or
    structure */
class FileWriter
{
public:
  virtual ~FileWriter() {}

  virtual void begin_section (const char* name) =0;
  virtual void end_section () =0;

  virtual void write_int    (const char* name, int) =0;
  virtual void write_float  (const char* name, float) =0;
  //virtual void write_color  (const char* name, const Color&) =0;
  virtual void write_bool   (const char* name, bool) =0;
  virtual void write_string (const char* name, const std::string&) =0;
  virtual void write_size   (const char* name, const Size&) = 0;
};


#endif

/* EOF */
