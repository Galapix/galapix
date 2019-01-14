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

#ifndef HEADER_GALAPIX_UTIL_TASK_HPP
#define HEADER_GALAPIX_UTIL_TASK_HPP

#include <exception>
#include <functional>

template<typename ReturnType>
class Task
{
public:
  template<typename Functor>
  Task(const Functor& functor) :
    m_functor(functor),
    m_return_value(),
    m_exception()
  {}

  void operator()()
  {
    try
    {
      m_return_value = m_functor();
    }
    catch(...)
    {
      m_exception = std::current_exception(); // NOLINT
    }
  }

  ReturnType get()
  {
    if (m_exception)
    {
      std::rethrow_exception(m_exception);
    }
    else
    {
      return m_return_value;
    }
  }

private:
  typename std::function<ReturnType ()> m_functor;
  ReturnType m_return_value;
  std::exception_ptr m_exception;
};

template<typename ReturnType>
class TaskHandle
{
public:
  TaskHandle() :
    m_return_value(),
    m_exception()
  {
  }

  ReturnType get()
  {
    if (m_exception)
    {
      std::rethrow_exception(m_exception);
    }
    else
    {
      return m_return_value;
    }
  }

private:
  ReturnType m_return_value;
  std::exception_ptr m_exception;
};

#endif

/* EOF */
