/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include "util/sexpr_file_reader.hpp"

#include <string.h>

#include "math/rgba.hpp"
#include "math/rect.hpp"
#include "util/file_reader_impl.hpp"

class SExprFileReaderImpl: public FileReaderImpl
{
public:
  std::shared_ptr<lisp::Lisp> sexpr;

  SExprFileReaderImpl(std::shared_ptr<lisp::Lisp> sexpr_)
    : sexpr(std::move(sexpr_))
  {
    assert(sexpr->get_type() == lisp::Lisp::TYPE_LIST &&
           sexpr->get_list_size() >= 1);

    for(size_t i = 1; i < sexpr->get_list_size(); ++i)
    { // iterate over subsections
      sexpr->get_list_elem(i);
    }
  }

  ~SExprFileReaderImpl() override
  {
    // FIXME: Do we have to free the lisp pointer here or outside of the code?
  }

  std::string get_name() const override
  {
    return sexpr->get_list_elem(0)->get_symbol();
  }

  bool read_int   (const char* name, int& v) const override
  {
    std::shared_ptr<lisp::Lisp> item = get_subsection_item(name);
    if (item && item->get_type() == lisp::Lisp::TYPE_INT)
    {
      v = item->get_int();
      return true;
    }
    return false;
  }

  bool read_float (const char* name, float& v) const override
  {
    std::shared_ptr<lisp::Lisp> item = get_subsection_item(name);
    if (item)
    {
      if (item->get_type() == lisp::Lisp::TYPE_FLOAT)
      {
        v = item->get_float();
        return true;
      }
      else if (item->get_type() == lisp::Lisp::TYPE_INT)
      {
        v = static_cast<float>(item->get_int());
        return true;
      }
      else
      {
        return false;
      }
    }
    return false;
  }

  bool read_bool  (const char* name, bool& v) const override
  {
    std::shared_ptr<lisp::Lisp> item = get_subsection_item(name);
    if (item && item->get_type() == lisp::Lisp::TYPE_BOOL)
    {
      v = item->get_bool();
      return true;
    }
    else if (item && item->get_type() == lisp::Lisp::TYPE_INT)
    {
      v = item->get_int();
      return true;
    }
    return false;
  }

  bool read_string(const char* name, std::string& v) const override
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub)
    {
      v = "";
      for(size_t i = 1; i < sub->get_list_size(); ++i)
      {
        std::shared_ptr<lisp::Lisp> item = sub->get_list_elem(i);
        if (item->get_type() == lisp::Lisp::TYPE_STRING)
        {
          v += item->get_string();
        }
        else if (item->get_type() == lisp::Lisp::TYPE_SYMBOL)
        {
          v += item->get_symbol();
        }
      }
      return true;
    }
    return false;
  }

  bool read_size(const char* name, Size& v) const override
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub && sub->get_list_size() == 3)
    {
      v.width  = sub->get_list_elem(1)->get_int();
      v.height = sub->get_list_elem(2)->get_int();
      return true;
    }
    return false;
  }

  bool read_vector2i(const char* name, Vector2i& v) const override
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub && sub->get_list_size() == 3)
    {
      v.x = sub->get_list_elem(1)->get_int();
      v.y = sub->get_list_elem(2)->get_int();
      return true;
    }
    return false;
  }

  bool read_vector2f(const char* name, Vector2f& v) const override
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub && sub->get_list_size() == 3)
    {
      v.x = sub->get_list_elem(1)->get_float();
      v.y = sub->get_list_elem(2)->get_float();
      return true;
    }
    return false;
  }

  bool read_rect(const char* name, Rect& rect) const override
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub && sub->get_list_size() == 5)
    {
      rect.left   = sub->get_list_elem(1)->get_int();
      rect.top    = sub->get_list_elem(2)->get_int();
      rect.right  = sub->get_list_elem(3)->get_int();
      rect.bottom = sub->get_list_elem(4)->get_int();
      return true;
    }
    return false;
  }

  bool read_rgba(const char* name, RGBA& v) const override
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub && sub->get_list_size() == 5)
    {
      v = RGBA(static_cast<uint8_t>(sub->get_list_elem(1)->get_float() * 255),
               static_cast<uint8_t>(sub->get_list_elem(2)->get_float() * 255),
               static_cast<uint8_t>(sub->get_list_elem(3)->get_float() * 255),
               static_cast<uint8_t>(sub->get_list_elem(4)->get_float() * 255));
      return true;
    }
    return false;
  }

  bool read_section(const char* name, FileReader& v) const override
  {
    std::shared_ptr<lisp::Lisp> cur = get_subsection(name);
    if (cur)
    {
      v = SExprFileReader(cur);
      return true;
    }
    return false;
  }

  std::vector<FileReader> get_sections() const override
  {
    std::vector<FileReader> lst;
    for(size_t i = 1; i < sexpr->get_list_size(); ++i)
    { // iterate over subsections
      lst.push_back(SExprFileReader(sexpr->get_list_elem(i)));
    }
    return lst;
  }

  std::vector<std::string> get_section_names() const override
  {
    std::vector<std::string> lst;

    for(size_t i = 1; i < sexpr->get_list_size(); ++i)
    { // iterate over subsections
      std::shared_ptr<lisp::Lisp> sub = sexpr->get_list_elem(i);
      lst.push_back(sub->get_list_elem(0)->get_symbol());
    }

    return lst;
  }

private:
  std::shared_ptr<lisp::Lisp> get_subsection_item(const char* name) const
  {
    std::shared_ptr<lisp::Lisp> sub = get_subsection(name);
    if (sub && sub->get_list_size() == 2)
    {
      return sub->get_list_elem(1);
    }
    return std::shared_ptr<lisp::Lisp>();
  }

  std::shared_ptr<lisp::Lisp> get_subsection(const char* name) const
  {
    for(size_t i = 1; i < sexpr->get_list_size(); ++i)
    { // iterate over subsections
      std::shared_ptr<lisp::Lisp> sub = sexpr->get_list_elem(i);
      if (strcmp(sub->get_list_elem(0)->get_symbol(), name) == 0)
        return sub;
    }
    return std::shared_ptr<lisp::Lisp>();
  }

};

SExprFileReader::SExprFileReader(std::shared_ptr<lisp::Lisp> lisp)
  : FileReader(std::shared_ptr<FileReaderImpl>(new SExprFileReaderImpl(std::move(lisp))))
{
}

/* EOF */
