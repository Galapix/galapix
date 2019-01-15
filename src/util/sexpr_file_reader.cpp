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

#include "util/sexpr_file_reader.hpp"

#include <string.h>

#include "math/rgba.hpp"
#include "math/rect.hpp"
#include "util/file_reader_impl.hpp"

class SExprFileReaderImpl: public FileReaderImpl
{
public:
  SExprFileReaderImpl(sexp::Value sx) :
    m_sx(std::move(sx))
  {
    assert(m_sx.is_array() && !m_sx.as_array().empty());
  }

  ~SExprFileReaderImpl() override
  {
  }

  std::string get_name() const override
  {
    return m_sx.as_array()[0].as_string();
  }

  bool read_int(const char* name, int& v) const override
  {
    sexp::Value const* item = get_subsection_item(name);
    if (item && item->is_integer())
    {
      v = item->as_int();
      return true;
    }
    return false;
  }

  bool read_float(const char* name, float& v) const override
  {
    sexp::Value const* item = get_subsection_item(name);
    if (item && item->is_real())
    {
      v = item->as_float();
      return true;
    }
    else
    {
      return false;
    }
  }

  bool read_bool  (const char* name, bool& v) const override
  {
    sexp::Value const* item = get_subsection_item(name);
    if (item && item->is_boolean())
    {
      v = item->as_bool();
      return true;
    }
    else if (item && item->is_integer())
    {
      v = item->as_int();
      return true;
    }
    return false;
  }

  bool read_string(const char* name, std::string& v) const override
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub)
    {
      v = "";
      for(size_t i = 1; i < sub->as_array().size(); ++i)
      {
        sexp::Value const& item = sub->as_array()[i];
        if (item.is_string() || item.is_symbol())
        {
          v += item.as_string();
        }
      }
      return true;
    }
    return false;
  }

  bool read_size(const char* name, Size& v) const override
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub && sub->as_array().size() == 3)
    {
      v.width  = sub->as_array()[1].as_int();
      v.height = sub->as_array()[2].as_int();
      return true;
    }
    return false;
  }

  bool read_vector2i(const char* name, Vector2i& v) const override
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub && sub->as_array().size() == 3)
    {
      v.x = sub->as_array()[1].as_int();
      v.y = sub->as_array()[2].as_int();
      return true;
    }
    return false;
  }

  bool read_vector2f(const char* name, Vector2f& v) const override
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub && sub->as_array().size() == 3)
    {
      v.x = sub->as_array()[1].as_float();
      v.y = sub->as_array()[2].as_float();
      return true;
    }
    return false;
  }

  bool read_rect(const char* name, Rect& rect) const override
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub && sub->as_array().size() == 5)
    {
      rect.left = sub->as_array()[1].as_int();
      rect.top = sub->as_array()[2].as_int();
      rect.right = sub->as_array()[3].as_int();
      rect.bottom = sub->as_array()[4].as_int();
      return true;
    }
    return false;
  }

  bool read_rgba(const char* name, RGBA& v) const override
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub && sub->as_array().size() == 5)
    {
      v = RGBA(static_cast<uint8_t>(sub->as_array()[1].as_float() * 255),
               static_cast<uint8_t>(sub->as_array()[2].as_float() * 255),
               static_cast<uint8_t>(sub->as_array()[3].as_float() * 255),
               static_cast<uint8_t>(sub->as_array()[4].as_float() * 255));
      return true;
    }
    return false;
  }

  bool read_section(const char* name, FileReader& v) const override
  {
    sexp::Value const* cur = get_subsection(name);
    if (cur)
    {
      v = SExprFileReader(*cur);
      return true;
    }
    return false;
  }

  std::vector<FileReader> get_sections() const override
  {
    std::vector<FileReader> lst;
    for(size_t i = 1; i < m_sx.as_array().size(); ++i)
    {
      lst.push_back(SExprFileReader(m_sx.as_array()[i]));
    }
    return lst;
  }

  std::vector<std::string> get_section_names() const override
  {
    std::vector<std::string> lst;

    for(size_t i = 1; i < m_sx.as_array().size(); ++i)
    {
      sexp::Value const& sub = m_sx.as_array()[i];
      lst.push_back(sub.as_array()[0].as_string());
    }

    return lst;
  }

private:
  sexp::Value const* get_subsection_item(const char* name) const
  {
    sexp::Value const* sub = get_subsection(name);
    if (sub && sub->is_array() && sub->as_array().size() == 2)
    {
      return &sub->as_array()[1];
    }
    return nullptr;
  }

  sexp::Value const* get_subsection(const char* name) const
  {
    assert(m_sx.is_array());
    for(size_t i = 1; i < m_sx.as_array().size(); ++i)
    { // iterate over subsections
      sexp::Value const& sub = m_sx.as_array()[i];
      if (sub.as_array()[0].as_string() == name) {
        return &sub;
      }
    }
    return nullptr;
  }

private:
  sexp::Value const m_sx;
};

SExprFileReader::SExprFileReader(sexp::Value sx) :
  FileReader(std::shared_ptr<FileReaderImpl>(new SExprFileReaderImpl(std::move(sx))))
{
}

/* EOF */
