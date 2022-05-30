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

#ifndef HEADER_GALAPIX_UTIL_FAILABLE_HPP
#define HEADER_GALAPIX_UTIL_FAILABLE_HPP

#include <assert.h>

namespace galapix {

template<typename T>
class Failable
{
public:
  template<typename E>
  static Failable<T> from_exception(E const& e)
  {
    return Failable<T>(std::make_exception_ptr(e));
  }

  Failable() :
    m_initialized(false),
    m_eptr()
  {
  }

  Failable(std::exception_ptr const& eptr) :
    m_initialized(false),
    m_eptr(eptr)
  {
  }

  Failable(T const& obj) :
    m_initialized(false),
    m_eptr()
  {
    construct(obj);
  }

  Failable(Failable const& rhs) :
    m_initialized(false),
    m_eptr(rhs.m_eptr)
  {
    if (rhs.is_initialized())
    {
      construct(rhs.m_obj);
    }
  }

  Failable& operator=(Failable const& rhs)
  {
    if (this != &rhs)
    {
      m_eptr = rhs.m_eptr;

      if (rhs.is_initialized())
      {
        assign(rhs.m_obj);
      }
      else
      {
        reset();
      }
    }

    return *this;
  }

  ~Failable()
  {
    destroy();
  }

  bool is_initialized() const { return m_initialized ; }

  void set_exception(std::exception_ptr const& eptr)
  {
    m_eptr = eptr;
  }

  void reset()
  {
    destroy();
  }

  void reset(T const& data)
  {
    assign(data);
  }

  T const&  get() const
  {
    if (m_eptr != nullptr)
    {
      std::rethrow_exception(m_eptr);
    }
    else
    {
      return m_obj;
    }
  }

  T& get()
  {
    if (m_eptr != nullptr)
    {
      std::rethrow_exception(m_eptr);
    }
    else
    {
      return m_obj;
    }
  }

private:
  void construct(T const& rhs)
  {
    assert(m_initialized == false);

    new (&m_obj) T(rhs);
    m_initialized = true;
  }

  void assign(T const& rhs)
  {
    if (is_initialized())
    {
      m_obj = rhs;
    }
    else
    {
      construct(rhs);
    }
  }

  void destroy()
  {
    if (is_initialized())
    {
      m_obj.~T();
    }
  }

private:
  union {
    char m_dummy[sizeof(T)];
    T m_obj;
  };

  bool m_initialized;

  std::exception_ptr m_eptr;
};

} // namespace galapix

#endif

/* EOF */
