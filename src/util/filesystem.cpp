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

#include <fstream>
#include <future>
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
#include <surf/software_surface.hpp>
#include <surf/software_surface_factory.hpp>

#include "archive/archive_loader.hpp"
#include "archive/archive_manager.hpp"
#include "archive/rar.hpp"
#include "archive/seven_zip.hpp"
#include "archive/tar.hpp"
#include "archive/zip.hpp"
#include "galapix/app.hpp"
#include "util/filesystem.hpp"
#include "util/raise_exception.hpp"

std::string Filesystem::home_directory;

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

    raise_runtime_error("Filesystem::find_exe(): Couldn't find " + name + " in PATH");
  }
  else
  {
    raise_runtime_error("Filesystem::find_exe(): Couldn't get PATH environment variable");
  }
}

bool
Filesystem::exist(const std::string& pathname)
{
  return access(pathname.c_str(), F_OK) == 0;
}

bool
Filesystem::is_directory(const std::string& pathname)
{
  struct stat buf;
  stat(pathname.c_str(), &buf);
  return S_ISDIR(buf.st_mode);
}

void
Filesystem::open_directory_recursivly(const std::string& pathname, std::vector<std::string>& lst)
{
  DIR* dp = ::opendir(pathname.c_str());

  if (dp == nullptr)
  {
    std::cout << "System: Couldn't open: " << pathname << std::endl;
  }
  else
  {
    dirent* de = nullptr;
    while ((de = ::readdir(dp)) != nullptr)
    {
      if (strcmp(de->d_name, ".")  != 0 &&
          strcmp(de->d_name, "..") != 0 &&
          strcmp(de->d_name, ".thumbnails") != 0 &&
          strcmp(de->d_name, ".xvpics") != 0) // FIXME: Doesn't really fit here
      {
        if (de->d_type == DT_DIR)
        { // Avoid stat'ing on file systems that don't need it
          open_directory_recursivly(pathname + "/" + de->d_name, lst);
        } // FIXME: Check for DT_UNKNOWN, DT_FILE, etc.
        else
        {
          std::string new_path = pathname + "/" + de->d_name;
          if (is_directory(new_path))
          {
            open_directory_recursivly(pathname + "/" + de->d_name, lst);
          }
          else
          {
            lst.push_back(new_path);
          }
        }
      }
    }

    closedir(dp);
  }
}

std::vector<std::string>
Filesystem::open_directory(const std::string& pathname)
{
  std::vector<std::string> dir_list;

  DIR* dp    = nullptr;
  dirent* de = nullptr;

  dp = ::opendir(pathname.c_str());

  if (dp == nullptr)
  {
    std::cout << "System: Couldn't open: " << pathname << std::endl;
  }
  else
  {
    while ((de = ::readdir(dp)) != nullptr)
    {
      if (strcmp(de->d_name, ".")  != 0 &&
          strcmp(de->d_name, "..") != 0) {
        dir_list.push_back(pathname + "/" + de->d_name);
      }
    }

    closedir(dp);
  }

  return dir_list;
}

void
Filesystem::init()
{
  char* home;
  if ((home = getenv("HOME")))
  {
    home_directory = home;
  }
  else
  {
    raise_runtime_error("Filesystem::init(): Couldn't get HOME environment variable");
  }

  mkdir(home_directory + "/.galapix");
}

void
Filesystem::mkdir(const std::string& pathname)
{
  if (!Filesystem::exist(pathname))
  {
    if (::mkdir(pathname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP) != 0)
    {
      raise_runtime_error("Filesystem::mkdir(): " + pathname + ": " + strerror(errno));
    }
    else
    {
      std::cout << "Filesystem::mkdir: " << pathname << std::endl;
    }
  }
}

void
Filesystem::deinit()
{
}

bool
Filesystem::has_extension(const std::string& filename, const std::string& ext)
{
  if (filename.length() >= ext.length())
  {
    return filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0;
  }
  else
  {
    return false;
  }
}

std::string
Filesystem::get_extension(const std::string& pathname)
{
  // FIXME: should take '/' into account and only check the actual
  // filename, instead of the whole pathname
  std::string::size_type p = pathname.rfind('.');
  std::string extension = pathname.substr(p+1, pathname.size() - p);

  std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

  if (extension == "gz" ||
      extension == "bz2")
  {
    p = pathname.rfind('.', p-1);
    extension = pathname.substr(p+1, pathname.size() - p);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    return extension;
  }
  else
  {
    return extension;
  }
}

void
Filesystem::copy_mtime(const std::string& from_filename, const std::string& to_filename)
{
  struct stat stat_buf;
  if (stat(from_filename.c_str(), &stat_buf) != 0)
  {
    raise_runtime_error("Filesystem::copy_mtime(): " + from_filename + ": " + strerror(errno));
  }

  struct utimbuf time_buf;
  time_buf.actime  = stat_buf.st_atime;
  time_buf.modtime = stat_buf.st_mtime;

  if (utime(to_filename.c_str(), &time_buf) != 0)
  {
    int err = errno;
    std::cout << "Filesystem:copy_mtime: " << to_filename << ": " << strerror(err) << std::endl;
  }
}

std::string
Filesystem::get_magic(const std::string& filename)
{
  char buf[512];
  std::ifstream in(filename, std::ios::binary);
  if (!in)
  {
    int err = errno;
    raise_exception(std::runtime_error, filename << ": couldn't open file: " << strerror(err));
  }
  else
  {
    if (in.read(buf, sizeof(buf)).bad())
    {
      raise_exception(std::runtime_error, filename << ": failed to read " << sizeof(buf) << " bytes");
    }
    else
    {
      return std::string(buf, static_cast<size_t>(in.gcount()));
    }
  }
}

size_t
Filesystem::get_size(const std::string& filename)
{
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) != 0)
  {
    raise_runtime_error("Filesystem::get_size(): " + filename + ": " + strerror(errno));
  }
  return static_cast<size_t>(stat_buf.st_size);
}

time_t
Filesystem::get_mtime(const std::string& filename)
{
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) != 0)
  {
    raise_runtime_error("Filesystem::get_mtime(): " + filename + ": " + strerror(errno));
  }
  return stat_buf.st_mtime;
}

// static bool has_prefix(const std::string& lhs, const std::string rhs)
// {
//   if (lhs.length() < rhs.length())
//     return false;
//   else
//     return lhs.compare(0, rhs.length(), rhs) == 0;
// }

void
Filesystem::generate_image_file_list(const std::string& pathname, std::vector<URL>& file_list)
{
  if (!exist(pathname))
  {
    std::cout << "Filesystem::generate_jpeg_file_list(): " << pathname << " does not exist" << std::endl;
  }
  else
  {
    // generate a list of all the files in the directories
    std::vector<std::string> lst;

    if (is_directory(pathname))
    {
      open_directory_recursivly(pathname, lst);
    }
    else
    {
      lst.push_back(pathname);
    }

    // check the file list for valid entries, if entries are archives,
    // get a file list from them
    std::vector<std::future<std::vector<URL>>> archive_tasks;
    for(std::vector<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
    {
      URL url = URL::from_filename(*i);

      try
      {
        if (g_app.archive().is_archive(*i))
        {
          archive_tasks.push_back(std::async([i, url]() -> std::vector<URL> {
                std::vector<URL> sub_file_list;

                const ArchiveLoader* loader;
                const auto& files = g_app.archive().get_filenames(*i, &loader);
                for(const auto& file: files)
                {
                  URL archive_url = URL::from_string(url.str() + "//" + loader->str() + ":" + file);
                  if (g_app.surface_factory().has_supported_extension(archive_url.str()))
                  {
                    sub_file_list.push_back(archive_url);
                  }
                }

                return sub_file_list;
              }));
        }
        else if (has_extension(*i, ".galapix"))
        {
          file_list.push_back(url);
        }
        else if (has_extension(*i, "ImageProperties.xml"))
        {
          file_list.push_back(url);
        }
        else if (url.get_protocol() == "builtin")
        {
          file_list.push_back(url);
        }
        else if (g_app.surface_factory().has_supported_extension(url.str()))
        {
          file_list.push_back(url);
        }
        else
        {
          //log_debug << "Filesystem::generate_image_file_list(): ignoring " << *i << std::endl;
        }
      }
      catch(const std::exception& err)
      {
        log_warn(err.what());
      }
    }

    for(auto& task: archive_tasks)
    {
      try
      {
        const auto& sub_lst = task.get();
        file_list.insert(file_list.end(), sub_lst.begin(), sub_lst.end());
      }
      catch(const std::exception& err)
      {
        log_warn("Warning: {}", err.what());
      }
    }
  }
}

std::string
Filesystem::realpath_system(const std::string& pathname)
{
  char* result = ::realpath(pathname.c_str(), nullptr);
  std::string res = result;
  free(result);

  return res;
}

std::string
Filesystem::realpath_fast(const std::string& pathname)
{
  std::string fullpath;
  std::string drive;

  if (!pathname.empty() && pathname[0] == '/')
  {
    fullpath = pathname;
  }
#ifdef WIN32
  else if (pathname.size() > 2 && pathname[1] == ':' && pathname[2] == '/')
  {
    drive = pathname.substr(0, 2);
    fullpath = pathname;
  }
#endif
  else
  {
    char buf[PATH_MAX];
    if (getcwd(buf, PATH_MAX) == nullptr)
    {
      std::cout << "System::realpath: Error: couldn't getcwd()" << std::endl;
      return pathname;
    }
#ifdef WIN32
    for (char *p = buf; *p; ++p)
    {
      if (*p == '\\')
        *p = '/';
    }
    drive.assign(buf, 2);
#endif

    fullpath = fullpath + buf + "/" + pathname;
  }

  std::string result;
  std::string::reverse_iterator last_slash = fullpath.rbegin();
  int skip = 0;
  // /foo/bar/../../bar/baz/
  //std::cout << "fullpath: '" << fullpath << "'" << std::endl;
  for(std::string::reverse_iterator i = fullpath.rbegin(); i != fullpath.rend(); ++i)
  { // FIXME: Little crude and hackish
    if (*i == '/')
    {
      std::string dir(last_slash, i);
      //std::cout << "'" << dir << "'" << std::endl;
      if (dir == ".." || dir == "/..")
      {
        skip += 1;
      }
      else if (dir == "." || dir == "/." || dir.empty() || dir == "/")
      {
        // pass
      }
      else
      {
        if (skip == 0) {
          result += dir;
        } else {
          skip -= 1;
        }
      }

      last_slash = i;
    }
  }

  return drive + "/" + std::string(result.rbegin(), result.rend());
}

std::string
Filesystem::realpath(const std::string& pathname)
{
  return realpath_fast(pathname);
}

void
Filesystem::readlines_from_file(const std::string& pathname, std::vector<std::string>& lst)
{
  std::ifstream in(pathname.c_str());

  if (!in)
  {
    raise_runtime_error("Filesystem::readlines_from_file(): Couldn't open file: " + pathname);
  }
  else
  {
    std::string line;
    while(std::getline(in, line))
    {
      lst.push_back(line);
    }
    in.close();
  }
}

void
Filesystem::remove(const std::string& filename)
{
  if (unlink(filename.c_str()) < 0)
  {
    std::ostringstream str;
    str << "Filesystem::remove: " << strerror(errno);
    raise_runtime_error(str.str());
  }
}

/* EOF */
