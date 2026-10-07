// babyxml - tiny parser for XML-like dialect
// Copyright (C) 2009-2020 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_BABYXML_READER_HPP
#define HEADER_BABYXML_READER_HPP

#include <string>
#include <vector>

#include "babyxml.hpp"

class BabyXML;

namespace babyxml {

class Reader
{
public:
  Reader(std::string_view text);

  std::vector<Node> parse();

private:
  void skip_space();
  void read_char(char expect);
  void read_element();
  std::string read_string();
  char read_entity();
  std::vector<Attribute> read_attributes();
  std::string read_identifer();
  bool eof() const;
  char look_ahead();
  void incr_cursor();

private:
  std::string m_text;
  std::vector<Node> m_nodes;
  int m_cursor;
  int m_line_no;
  int m_column_no;

private:
  Reader(const Reader&) = delete;
  Reader& operator= (const Reader&) = delete;
};

} // namespace babyxml

#endif

/* EOF */
