// UITest - Simple testing framework for interactive tests
// Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_UITEST_HPP
#define HEADER_UITEST_HPP

#include <boost/tokenizer.hpp>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace uitesting {

class UITest
{
public:
  UITest() {}
  virtual ~UITest() {}
  virtual void TestBody(const std::vector<std::string>& args) = 0;
};

class TestInfo
{
public:
  std::string m_name;
  std::string m_class;
  std::string m_function;
  std::string m_args_str;
  std::vector<std::string> m_args;
  std::string m_doc;
  std::function<std::unique_ptr<UITest> ()> m_factory;
};

std::vector<std::unique_ptr<TestInfo> >& g_tests();

TestInfo*
register_test(const std::string& _class, const std::string& _func,
              const std::function<std::unique_ptr<UITest> ()>& factory,
              const std::string& args_str = {}, const std::string& doc = {});

TestInfo*
find_testcase(const std::string& testcase);

} // namespace uitesting

#define UITEST(_class, _func, ...)                                      \
  class _class##_##_func : public ::uitesting::UITest                   \
  {                                                                     \
  public:                                                               \
    static ::uitesting::TestInfo* const info __attribute__ ((unused));  \
    void TestBody(const std::vector<std::string>& args) override;       \
  };                                                                    \
  ::uitesting::TestInfo* const _class##_##_func::info                   \
  = ::uitesting::register_test(#_class, #_func,                         \
                  []() {                                                \
                    return std::make_unique<_class##_##_func>();        \
                  },                                                    \
                  __VA_ARGS__);                                         \
  void _class##_##_func::TestBody(const std::vector<std::string>& args)

#endif

/* EOF */
