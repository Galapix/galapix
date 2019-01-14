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

#include "util/file_reader.hpp"

#include "lisp/parser.hpp"
#include "util/file_reader_impl.hpp"
#include "util/sexpr_file_reader.hpp"
#include "util/url.hpp"

FileReader::FileReader(std::shared_ptr<FileReaderImpl> const& impl_) :
  impl(impl_)
{
}

FileReader::FileReader() :
  impl()
{
}

std::string
FileReader::get_name() const
{
  if (impl.get())
    return impl->get_name();
  else
    return "";
}

bool
FileReader::read_int(const char* name, int& value) const
{
  if (impl.get())
    return impl->read_int(name, value);
  else
    return false;
}

bool
FileReader::read_float (const char* name, float& value) const
{
  if (impl.get())
    return impl->read_float(name, value);
  else
    return false;
}

bool
FileReader::read_bool  (const char* name, bool& value) const
{
  if (impl.get())
    return impl->read_bool(name, value);
  else
    return false;
}

bool
FileReader::read_string(const char* name, std::string& value) const
{
  if (impl.get())
    return impl->read_string(name, value);
  else
    return false;
}

bool
FileReader::read_url(const char* name, URL& value) const
{
  if (impl.get())
  {
    std::string filename;
    if (impl->read_string(name, filename))
    {
      value = URL::from_string(filename);
      return true;
    }
    else
    {
      return false;
    }
  }
  else
    return false;
}

bool
FileReader::read_rgba(const char* name, RGBA& value) const
{
  if (impl.get())
    return impl->read_rgba(name, value);
  else
    return false;
}

bool
FileReader::read_size  (const char* name, Size& value) const
{
  if (impl.get())
    return impl->read_size(name, value);
  else
    return false;
}

bool
FileReader::read_vector2i(const char* name, Vector2i& value) const
{
  if (impl.get())
    return impl->read_vector2i(name, value);
  else
    return false;
}

bool
FileReader::read_vector2f(const char* name, Vector2f& value) const
{
  if (impl.get())
    return impl->read_vector2f(name, value);
  else
    return false;
}

bool
FileReader::read_rect(const char* name, Rect& value)    const
{
  if (impl.get())
    return impl->read_rect(name, value);
  else
    return false;
}

bool
FileReader::read_section(const char* name, FileReader& reader) const
{
  if (impl.get())
    return impl->read_section(name, reader);
  else
    return false;
}

std::vector<std::string>
FileReader::get_section_names() const
{
  if (impl.get())
    return impl->get_section_names();
  else
    return std::vector<std::string>();
}

std::vector<FileReader>
FileReader::get_sections() const
{
  if (impl.get())
    return impl->get_sections();
  else
    return std::vector<FileReader>();
}

int
FileReader::get_num_sections() const
{
  return int(impl->get_sections().size());
}

FileReader
FileReader::read_section(const char* name)   const
{
  FileReader reader;
  read_section(name, reader);
  return reader;
}

FileReader
FileReader::parse(const std::string& filename)
{
  std::shared_ptr<lisp::Lisp> sexpr = lisp::Parser::parse(filename);
  if (sexpr)
  {
    return SExprFileReader(sexpr->get_list_elem(0));
  }
  else
  {
    return {};
  }
}

/* EOF */
