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

#ifndef HEADER_GALAPIX_UTIL_WEAK_FUNCTOR_HPP
#define HEADER_GALAPIX_UTIL_WEAK_FUNCTOR_HPP

#include <iostream>
#include <memory>

#include "util/log.hpp"

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

template<class F, class C>
class WeakFunctor
{
private:
  F m_func;
  std::weak_ptr<typename C::element_type> m_obj;

public: 
  WeakFunctor(F func, C obj) :
    m_func(func),
    m_obj(obj)
  {}

  void operator()()
  {
    std::shared_ptr<typename C::element_type> obj = m_obj.lock();
    if (obj)
    {
      m_func(obj);
    }
    else
    {
      log_debug << "WeakFunctor(): object deleted, not calling callback" << std::endl;
    }
  }

  template<class A1>
  void operator()(A1 a)
  {
    std::shared_ptr<typename C::element_type> obj = m_obj.lock();
    if (obj)
    {
      m_func(obj, a);
    }
    else
    {
      log_debug << "WeakFunctor(): object deleted, not calling callback" << std::endl;
    }
  }

  template<class A1, class A2>
  void operator()(A1 a1, A2 a2)
  {
    std::shared_ptr<typename C::element_type> obj = m_obj.lock();
    if (obj)
    {
      m_func(obj, a1, a2);
    }
    else
    {
      log_debug << "WeakFunctor(): object deleted, not calling callback" << std::endl;
    }
  }
};

template<class F, class C>
WeakFunctor<F, C> weak(F func, C obj)
{
  return WeakFunctor<F, C>(func, obj);
}

#endif

/* EOF */
