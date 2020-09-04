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

#include "archive/tar.hpp"

#include <stdexcept>

#include "util/exec.hpp"
#include "util/raise_exception.hpp"

std::vector<std::string>
Tar::get_filenames(const std::string& tar_filename)
{
  Exec tar("tar");
  tar.arg("--list").arg("--file").arg(tar_filename);
  if (tar.exec() == 0)
  {
    std::vector<std::string> lst;
    auto const& stdout_lst = tar.get_stdout();
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
    raise_runtime_error("Tar::get_filenames(): " + std::string(tar.get_stderr().begin(), tar.get_stderr().end()));
    return std::vector<std::string>();
  }
}

Blob
Tar::get_file(const std::string& tar_filename, const std::string& filename)
{
  Exec tar("tar");
  tar.arg("--extract").arg("--to-stdout").arg("--file").arg(tar_filename).arg(filename);
  if (tar.exec() == 0)
  {
    // FIXME: Unneeded copy of data
    return Blob::copy(tar.get_stdout());
  }
  else
  {
    raise_runtime_error("Tar::get_file(): " + tar.str() + "\n" + std::string(tar.get_stderr().begin(), tar.get_stderr().end()));
  }
}

void
Tar::extract(const std::string& tar_filename, const std::string& target_directory)
{
  Exec tar("tar");
  tar
    .arg("--extract")
    .arg("--directory").arg(target_directory)
    .arg("--file").arg(tar_filename);
  if (tar.exec() != 0)
  {
    raise_runtime_error(tar.str() + "\n" + std::string(tar.get_stderr().begin(), tar.get_stderr().end()));
  }
}

/* EOF */
