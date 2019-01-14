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

#include "archive/seven_zip.hpp"

#include <stdexcept>

#include "util/exec.hpp"
#include "util/raise_exception.hpp"

namespace {
bool has_prefix(const std::string& lhs, const std::string& rhs)
{
  if (lhs.length() < rhs.length())
    return false;
  else
    return lhs.compare(0, rhs.length(), rhs) == 0;
 }
} // namespace

std::vector<std::string>
SevenZip::get_filenames(const std::string& zip_filename)
{
  std::vector<std::string> lst;

  Exec zip("7zr");
  zip.arg("l").arg("-slt");
  zip.arg(zip_filename);

  if (zip.exec() != 0)
  {
    raise_runtime_error("SevenZip::get_filenames(): " + std::string(zip.get_stderr().begin(), zip.get_stderr().end()));
    return std::vector<std::string>();
  }
  else
  {
    const std::vector<char>& stdout_lst = zip.get_stdout();
    std::vector<char>::const_iterator line_start = stdout_lst.begin();
    bool parse_files = false;
    std::string file_start = "----------";
    for(std::vector<char>::const_iterator i = stdout_lst.begin(); i != stdout_lst.end(); ++i)
    {
      if (*i == '\n')
      {
        std::string line(line_start, i);
        line_start = i + 1;

        if (!parse_files)
        {
          if (line == file_start)
          {
            parse_files = true;
          }
        }
        else
        {
          if (has_prefix(line, "Path = "))
          {
            lst.push_back(line.substr(7));
          }
        }
      }
    }
  }
  return lst;
}

Blob
SevenZip::get_file(const std::string& zip_filename, const std::string& filename)
{
  Exec zip("7zr");
  zip.arg("x").arg("-so");
  zip.arg(zip_filename);
  zip.arg(filename);

  if (zip.exec() == 0)
  {
    // FIXME: Unneeded copy of data
    return Blob::copy(&*zip.get_stdout().begin(), zip.get_stdout().size());
  }
  else
  {
    raise_runtime_error(zip.str() + "\n" + std::string(zip.get_stderr().begin(), zip.get_stderr().end()));
    return {};
  }
}

void
SevenZip::extract(const std::string& archive, const std::string& target_directory)
{
  Exec zip("7zr");
  zip.arg("x").arg("-o" + target_directory);
  zip.arg(archive);
  if (zip.exec() != 0)
  {
    raise_runtime_error(zip.str() + "\n" + std::string(zip.get_stderr().begin(), zip.get_stderr().end()));
  }
}

/* EOF */
