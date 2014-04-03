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

#include "util/sexpr_file_writer.hpp"

#include <iostream>
#include <map>

SExprFileWriter::SExprFileWriter(std::ostream& out_)
  : out(&out_), level(0)
{
}

SExprFileWriter::~SExprFileWriter()
{
}

std::string
SExprFileWriter::indent()
{
  return std::string(level*2, ' ');
}

void
SExprFileWriter::begin_section(const char* name)
{
  (*out) << "\n" << indent() << "(" << name << " ";
  ++level;
}

void
SExprFileWriter::end_section()
{
  --level;
  (*out) << ")";
}

void
SExprFileWriter::write_int(const char* name, int value)
{
  (*out) << "\n" << indent() << "(" << name << " " << value << ")";
}

void
SExprFileWriter::write_float(const char* name, float value)
{
  (*out) << "\n" << indent() << "(" << name << " " << value << ")";
}

void
SExprFileWriter::write_rgba(const char* name, const RGBA& rgba)
{
  (*out) << "\n" << indent() << "(" << name << " "
         << static_cast<float>(rgba.r)/255.0f << " "
         << static_cast<float>(rgba.g)/255.0f << " "
         << static_cast<float>(rgba.b)/255.0f << " "
         << static_cast<float>(rgba.a)/255.0f << ")";
}

void
SExprFileWriter::write_bool(const char* name, bool value)
{
  (*out) << "\n" << indent() << "(" << name << " " << (value ? "#t" : "#f") << ")";
}

void
SExprFileWriter::write_string(const char* name, const std::string& value)
{
  // Perform basic XML encoding (turns apostrophes into &apos;, etc.
  std::string new_value = value;
  std::string::size_type pos;

  std::map<std::string, std::string> replacements;

  replacements["\""] = "\\\"";
  replacements["\\"] = "\\\\";

  for (std::map<std::string, std::string>::iterator i = replacements.begin();
       i != replacements.end(); ++i)
  {
    for (pos = new_value.find(i->first); pos != std::string::npos; pos = new_value.find(i->first))
    {
      // Replace character with encoding characters
      new_value.replace(pos, 1, i->second);
    }
  }

  (*out) << "\n" << indent() << "(" << name << " \"" << new_value << "\")";
}

void
SExprFileWriter::write_size(const char* name, const Size& size)
{
  (*out) << "\n" << indent() << "(" << name << " " << size.width << " " << size.height << ")";
}

/* EOF */
