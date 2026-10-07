// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <fstream>
#include <future>
#include <cctype>
#include <string>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <utime.h>
#include <sstream>
#include <algorithm>
#include <logmich/log.hpp>

#include "util/filesystem.hpp"

namespace surf {

std::string
Filesystem::find_exe(const std::string& name)
{
  char* path_c = getenv("PATH");
  if (path_c)
  {
    const char* delim = ":";
    char* path = strdup(path_c);
    char* state;

    for(char* p = strtok_r(path, delim, &state); p != nullptr; p = strtok_r(nullptr, delim, &state))
    {
      std::ostringstream fullpath;
      fullpath << p << "/" << name;
      if (access(fullpath.str().c_str(), X_OK) == 0)
      {
        free(path);
        return fullpath.str();
      }
    }

    free(path);

    throw std::runtime_error("Filesystem::find_exe(): Couldn't find " + name + " in PATH");
  }
  else
  {
    throw std::runtime_error("Filesystem::find_exe(): Couldn't get PATH environment variable");
  }
}

std::string
Filesystem::get_extension(std::filesystem::path const& path)
{
  // only look at the filename, not at the directories
  std::string const pathname = path.filename().string();

  std::string::size_type p = pathname.rfind('.');
  if (p == std::string::npos) {
    return {};
  }

  auto tolower = [](std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
  };

  std::string extension = tolower(pathname.substr(p + 1));

  if ((extension == "gz" || extension == "bz2") && p > 0)
  {
    std::string::size_type const p2 = pathname.rfind('.', p - 1);
    if (p2 != std::string::npos) {
      return tolower(pathname.substr(p2 + 1));
    }
  }

  return extension;
}

std::string
Filesystem::get_magic(std::filesystem::path const& filename)
{
  char buf[512];
  std::ifstream in(filename, std::ios::binary);
  if (!in)
  {
    int err = errno;
    throw std::runtime_error(filename.string() + ": couldn't open file: " + strerror(err));
  }
  else
  {
    if (in.read(buf, sizeof(buf)).bad())
    {
      throw std::runtime_error(filename.string() + ": failed to read " + std::to_string( sizeof(buf)) + " bytes");
    }
    else
    {
      return std::string(buf, static_cast<size_t>(in.gcount()));
    }
  }
}

} // namespace surf

/* EOF */
