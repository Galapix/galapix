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

#include "plugins/tar.hpp"
#include "plugins/zip.hpp"
#include "plugins/rar.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"
#include "util/filesystem.hpp"

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

    for(char* p = strtok_r(path, delim, &state); p != NULL; p = strtok_r(NULL, delim, &state))
    {
      std::ostringstream fullpath; 
      fullpath << p << "/" << name;
      if (access(fullpath.str().c_str(), X_OK) == 0)
      {
        return fullpath.str();
      }
    }
      
    free(path);

    throw std::runtime_error("Couldn't find " + name + " in PATH");
  }
  else
  {
    throw std::runtime_error("Couldn't get PATH environment variable");
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
    throw std::runtime_error("Couldn't get HOME environment variable");
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
      throw std::runtime_error("Filesystem::mkdir: " + pathname + ": " + strerror(errno));
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
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
  else
    return false;
}

void
Filesystem::copy_mtime(const std::string& from_filename, const std::string& to_filename)
{
  struct stat stat_buf;
  if (stat(from_filename.c_str(), &stat_buf) != 0)
  {
    throw std::runtime_error(from_filename + ": " + strerror(errno));
  }

  struct utimbuf time_buf; 
  time_buf.actime  = stat_buf.st_atime;
  time_buf.modtime = stat_buf.st_mtime;

  if (utime(to_filename.c_str(), &time_buf) != 0)
  {
    std::cout << "Filesystem:copy_mtime: " << to_filename << ": " << strerror(errno) << std::endl;
  }
}

unsigned int
Filesystem::get_size(const std::string& filename)
{
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) != 0)
  {
    throw std::runtime_error(filename + ": " + strerror(errno));
  } 
  return stat_buf.st_size; // Is this reliable? or should be use fopen() and ftell()?
}

unsigned int
Filesystem::get_mtime(const std::string& filename)
{
  struct stat stat_buf;
  if (stat(filename.c_str(), &stat_buf) != 0)
  {
    throw std::runtime_error(filename + ": " + strerror(errno));
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
    std::cout << "Filesystem::generate_jpeg_file_list: Error: " << pathname << " does not exist" << std::endl;
  }
  else
  {
    std::vector<std::string> lst;

    if (is_directory(pathname))
      open_directory_recursivly(pathname, lst);
    else
      lst.push_back(pathname);
  
    for(std::vector<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
    {
      URL url = URL::from_filename(*i);

      try 
      {
        if (has_extension(*i, ".rar") || has_extension(*i, ".rar.part") || has_extension(*i, ".cbr"))
        {
          const std::vector<std::string>& files = Rar::get_filenames(*i);
          for(std::vector<std::string>::const_iterator j = files.begin(); j != files.end(); ++j)
            file_list.push_back(URL::from_string(url.str() + "//rar:" + *j));
        }
        else if (has_extension(*i, ".zip") || has_extension(*i, ".cbz"))
        {
          const std::vector<std::string>& files = Zip::get_filenames(*i);
          for(std::vector<std::string>::const_iterator j = files.begin(); j != files.end(); ++j)
            file_list.push_back(URL::from_string(url.str() + "//zip:" + *j));
        }
        else if (has_extension(*i, ".tar") || has_extension(*i, ".tar.bz") || has_extension(*i, ".tar.gz") ||
                 has_extension(*i, ".tgz") || has_extension(*i, ".tbz"))
        {
          const std::vector<std::string>& files = Tar::get_filenames(*i);
          for(std::vector<std::string>::const_iterator j = files.begin(); j != files.end(); ++j)
            file_list.push_back(URL::from_string(url.str() + "//tar:" + *j));
        }
        else if (has_extension(*i, ".galapix"))
        {
          file_list.push_back(url);
        }
        else
        {
          if (SoftwareSurfaceFactory::get_fileformat(url) != SoftwareSurfaceFactory::UNKNOWN_FILEFORMAT)
          {
            file_list.push_back(url);
          }
        }
      } 
      catch(std::exception& err) 
      {
        std::cout << "Warning: " << err.what() << std::endl;
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
    throw std::runtime_error("Couldn't open file: " + pathname);
  
  std::string line;
  while(std::getline(in, line))
  {
    lst.push_back(line);
  }
  in.close();
}

/* EOF */
