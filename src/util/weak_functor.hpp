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

#ifndef HEADER_GALAPIX_UTIL_WEAK_FUNCTOR_HPP
#define HEADER_GALAPIX_UTIL_WEAK_FUNCTOR_HPP

#include <iostream>
#include <memory>

#include <logmich/log.hpp>

/**
 *  WeakFunctor allows you to create functors from objects that are
 *  stored in a shared_ptr<> by using a weak_ptr<>. When the object
 *  gets deleted before the functor is called, the callback will
 *  simply do nothing.
 *
 *  Use it like:
 *
 *   weak(std::bind(&Foobar::callme, _1), object);
 *
 */
template<typename F, typename C>
class WeakFunctor
{
public:
  WeakFunctor(const F& func, const C& obj) :
    m_func(func),
    m_obj(obj)
  {}

  template<typename... Args>
  void operator()(Args&&... args)
  {
    std::shared_ptr<typename C::element_type> obj = m_obj.lock();
    if (obj)
    {
      m_func(obj, std::forward<Args>(args)...);
    }
    else
    {
      log_debug("WeakFunctor(): object deleted, not calling callback");
    }
  }

private:
  F m_func;
  std::weak_ptr<typename C::element_type> m_obj;
};

template<typename F, typename C>
WeakFunctor<F, C> weak(F func, C obj)
{
  return WeakFunctor<F, C>(func, obj);
}

#endif

/* EOF */
