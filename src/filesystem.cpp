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
#include <boost/format.hpp>
//#include <attr/xattr.h>

#include "filesystem.hpp"

std::string Filesystem::home_directory;

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
              strcmp(de->d_name, "..") != 0)
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

void
Filesystem::generate_image_file_list(const std::string& pathname, std::vector<std::string>& file_list)
{
  std::vector<std::string> lst;
  if (!exist(pathname))
    {
      std::cout << "Filesystem::generate_jpeg_file_list: Error: " << pathname << " does not exist" << std::endl;
    }
  else
    {
      if (is_directory(pathname))
        open_directory_recursivly(pathname, lst);
      else
        lst.push_back(pathname);
  
      for(std::vector<std::string>::iterator i = lst.begin(); i != lst.end(); ++i)
        {
          // FIXME: Convert to lowercase before doing the compare
          if (Filesystem::has_extension(*i, ".jpg")  ||
              Filesystem::has_extension(*i, ".JPG")  ||
              Filesystem::has_extension(*i, ".jpe")  ||
              Filesystem::has_extension(*i, ".JPE")  ||
              Filesystem::has_extension(*i, ".JPEG") ||
              Filesystem::has_extension(*i, ".jpeg") ||
              Filesystem::has_extension(*i, ".PNG")  ||
              Filesystem::has_extension(*i, ".png"))
            {
              file_list.push_back(Filesystem::realpath(*i)); // realpath slow?
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
