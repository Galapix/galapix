//  Galapix - an image viewer for large image collections
//  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#include "job/semaphore.hpp"
#include "plugins/rar.hpp"
#include "plugins/seven_zip.hpp"
#include "plugins/tar.hpp"
#include "plugins/zip.hpp"
#include "util/archive_manager.hpp"
#include "util/archive_loader.hpp"
#include "util/filesystem.hpp"
#include "util/log.hpp"
#include "util/raise_exception.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"

std::string Filesystem::home_directory;

std::string
Filesystem::find_exe(const std::string& name_in_path, const std::string& envvar)
{
  char* const envvar_exe = getenv(envvar.c_str());
  if (envvar_exe != nullptr) {
    return envvar_exe;
  } else {
    return find_exe(name_in_path);
  }
}

std::string
Filesystem::find_exe(const std::string& name)
{
  char* path_c = getenv("PATH");
  if (path_c)
  {
    const char* delim = ":";
    char* path = strdup(path_c);
    char* state;

    for(char* p = strtok_r(path, delim, &state); p != NULL; p = strtok_r(NULL, delim, &state))
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

  if (dp == 0)
  {
    std::cout << "System: Couldn't open: " << pathname << std::endl;
  }
  else
  {
    dirent* de = 0;
    while ((de = ::readdir(dp)) != 0)
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

  DIR* dp    = 0;
  dirent* de = 0;

  dp = ::opendir(pathname.c_str());

  if (dp == 0)
  {
    std::cout << "System: Couldn't open: " << pathname << std::endl;
  }
  else
  {
    while ((de = ::readdir(dp)) != 0)
    {
      if (strcmp(de->d_name, ".")  != 0 &&
          strcmp(de->d_name, "..") != 0)
        dir_list.push_back(pathname + "/" + de->d_name);
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
    throw std::runtime_error("Filesystem::init(): Couldn't get HOME environment variable");
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
      throw std::runtime_error("Filesystem::mkdir(): " + pathname + ": " + strerror(errno));
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
Filesystem::has_extension(const std::string& str, const std::string& suffix)
{
  if (str.length() >= suffix.length())
  {
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
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
    throw std::runtime_error("Filesystem::copy_mtime(): " + from_filename + ": " + strerror(errno));
  }

  struct utimbuf time_buf; 
  time_buf.actime  = stat_buf.st_atime;
  time_buf.modtime = stat_buf.st_mtime;

  if (utime(to_filename.c_str(), &time_buf) != 0)
  {
    std::cout << "Filesystem:copy_mtime: " << to_filename << ": " << strerror(errno) << std::endl;
  }
}

std::string
Filesystem::get_magic(const std::string& filename)
{
  char buf[512];
  std::ifstream in(filename, std::ios::binary);
  if (!in)
  {
    raise_exception(std::runtime_error, filename << ": " << strerror(errno));
  }
  else
  {
    if (!in.read(buf, sizeof(buf)))
    {
      raise_exception(std::runtime_error, filename << ": " << strerror(errno));
    }
    else
    {
      return std::string(buf, in.gcount());
    }
  }
}

unsigned int
Filesystem::get_size(const std::string& filename)
{
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) != 0)
  {
    throw std::runtime_error("Filesystem::get_size(): " + filename + ": " + strerror(errno));
  } 
  return stat_buf.st_size; // Is this reliable? or should be use fopen() and ftell()?
}

unsigned int
Filesystem::get_mtime(const std::string& filename)
{
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) != 0)
  {
    throw std::runtime_error("Filesystem::get_mtime(): " + filename + ": " + strerror(errno));
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
    Semaphore task_limiter(std::thread::hardware_concurrency());
    std::vector<std::future<std::vector<URL>>> archive_tasks;
    for(std::vector<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
    {
      URL url = URL::from_filename(*i);
      std::cout << "Processing: " << *i << std::endl;
      try
      {
        if (ArchiveManager::current().is_archive(*i))
        {
          std::cout << "async\n";
          archive_tasks.push_back(
            std::async(
              std::launch::async,
              [i, url, &task_limiter]() -> std::vector<URL> {
                SemaphoreLock lock(task_limiter);

                std::vector<URL> sub_file_list;

                const ArchiveLoader* loader;
                const auto& files = ArchiveManager::current().get_filenames(*i, &loader);
                for(const auto& file: files)
                {
                  URL archive_url = URL::from_string(url.str() + "//" + loader->str() + ":" + file);
                  if (SoftwareSurfaceFactory::current().has_supported_extension(archive_url))
                  {
                    sub_file_list.push_back(archive_url);
                  }
                }

                return sub_file_list;
              }));
          std::cout << "async: done\n";
        }
        else if (has_extension(*i, ".galapix"))
        {
          file_list.push_back(url);
        }
        else if (has_extension(*i, "ImageProperties.xml"))
        {
          file_list.push_back(url);
        }
        else if (url.get_protocol() == "builtin" || url.get_protocol() == "buildin")
        {
          file_list.push_back(url);
        }
        else if (SoftwareSurfaceFactory::current().has_supported_extension(url))
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
        log_warning << "Warning: " << err.what() << std::endl;
      }
    }

    std::cout << "waiting for async tasks: " << archive_tasks.size() << std::endl;
    for(auto& task: archive_tasks)
    {
      try 
      {
        const auto& sub_lst = task.get();
        file_list.insert(file_list.end(), sub_lst.begin(), sub_lst.end());
      }
      catch(const std::exception& err)
      {
        log_warning << "Warning: " << err.what() << std::endl;
      }
    }
  }
}

std::string
Filesystem::realpath_system(const std::string& pathname)
{
  char* result = ::realpath(pathname.c_str(), NULL);
  std::string res = result;
  free(result);
  
  return res;
}

std::string
Filesystem::realpath_fast(const std::string& pathname)
{
  std::string fullpath;
  std::string drive;
  
  if (pathname.size() > 0 && pathname[0] == '/')
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
    if (getcwd(buf, PATH_MAX) == 0)
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
        if (skip == 0)
        {
          result += dir;
        }
        else
          skip -= 1;
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
    throw std::runtime_error("Filesystem::readlines_from_file(): Couldn't open file: " + pathname);
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
    throw std::runtime_error(str.str());
  }
}

/* EOF */
