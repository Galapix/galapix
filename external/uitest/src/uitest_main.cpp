//  UITest - Simple testing framework for interactive tests
//  Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <uitest/uitest.hpp>

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " TESTCASE [ARG]...\n"
              << "\nTest Cases:\n";
    for(const auto& testinfo : ::uitesting::g_tests())
    {
      std::cout << "  " << testinfo->m_class << "." << testinfo->m_function << " " << testinfo->m_args_str << std::endl;
    }
    return 0;
  }
  else
  {
    std::string testcase = argv[1];
    ::uitesting::TestInfo* testinfo = ::uitesting::find_testcase(testcase);
    if (!testinfo)
    {
      std::cerr << "Couldn't find test case '" << testcase << "'" << std::endl;
      return 1;
    }
    else
    {
      auto testobj = testinfo->m_factory();
      if (static_cast<int>(testinfo->m_args.size()) != argc - 2)
      {
        std::cerr << "error: incorrect number of arguments, expected " << testinfo->m_args.size() << " got " << (argc-2) << "\n"
                  << "Usage: " << argv[0] << " " << testinfo->m_name
                  << " " << testinfo->m_args_str << '\n'
                  << "  " << testinfo->m_doc << std::endl;
      }
      else
      {
        testobj->TestBody(std::vector<std::string>(argv+2, argv + argc));
      }
      return 0;
    }
  }
}

/* EOF */
