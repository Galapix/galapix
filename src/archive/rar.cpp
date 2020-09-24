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

#include "archive/rar.hpp"

#include <stdexcept>
#include <iostream>
#include <fstream>

#include "util/exec.hpp"

std::vector<std::string>
Rar::get_filenames(const std::string& rar_filename)
{
  Exec rar("rar");
  rar.arg("vb").arg("-p-").arg(rar_filename);
  if (rar.exec() == 0)
  {
    std::vector<std::string> lst;
    auto const& stdout_lst = rar.get_stdout();
    auto start = stdout_lst.begin();
    for(auto i = stdout_lst.begin(); i != stdout_lst.end(); ++i)
    {
      if (*i == '\n')
      {
        lst.push_back(std::string(start, i));
        start = i+1;
      }
    }
    return lst;
  }
  else
  {
    throw std::runtime_error(std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
    return std::vector<std::string>();
  }
}

Blob
Rar::get_file(const std::string& rar_filename, const std::string& filename)
{
  Exec rar("rar");
  rar.arg("p").arg("-inul").arg("-p-").arg(rar_filename).arg(filename);
  if (rar.exec() == 0)
  {
    // FIXME: Unneeded copy of data
    return Blob::copy(rar.get_stdout());
  }
  else
  {
    throw std::runtime_error(rar.str() + "\n" + std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
    return {};
  }
}

void
Rar::extract(const std::string& rar_filename, const std::string& target_directory)
{
  Exec rar("rar");
  rar.arg("x").arg("-inul").arg("-w" + target_directory).arg(rar_filename);
  if (rar.exec() != 0)
  {
    throw std::runtime_error(rar.str() + "\n" + std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
  }
}

/* EOF */
