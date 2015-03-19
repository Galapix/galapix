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

#include <uitest/uitest.hpp>
#include <iostream>
#include <stdexcept>

UITEST(ExampleTest, download, "HOST URL...",
       "Try to download the given URLs")
{
  for(const auto& arg : args)
  {
    std::cout << "HOST: " << arg << std::endl;
  }

  for(const auto& arg : rest)
  {
    std::cout << "Downloading from URL: " << arg << std::endl;
  }
}

UITEST(ExampleTest, other_test, "FILE",
       "Try to download the given URL")
{
  std::string file = args[0];
  std::cout << "Running a test on FILE: " << file << std::endl;
}

UITEST(ExampleTest, arg_cat, "ARG1 ARG2",
       "Concatenate the two arguments")
{
  std::string arg1 = args[0];
  std::string arg2 = args[1];
  std::cout << "Result: " << arg1 + arg2 << std::endl;
}

UITEST(ExampleTest, throw_exception, "MESSAGE",
       "Throw exception with MESSAGE")
{
  throw std::runtime_error(args[0]);
}

/* EOF */
