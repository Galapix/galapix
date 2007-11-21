/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

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
#if 0
std::string
Filesystem::getxattr(const std::string& pathname)
{
  char buf[2048];
  int len;
  if ((len = ::getxattr(pathname.c_str(), "user.griv.md5", buf, 2048)) < 0)
    {
      if (errno == ENOATTR)
        return "";
      else
        throw std::runtime_error("Couldn't get xattr for " + pathname);
    }

  return std::string(buf, len);
}
#endif
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

  mkdir(home_directory + "/.griv");
  mkdir(home_directory + "/.griv/cache");
  mkdir(home_directory + "/.griv/cache/by_url");

  std::string base = home_directory + "/.griv/cache/by_url";
  std::string hex  = "0123456789abcdef";
  int res[]  = { 16, 32, 64, 128, 256, 512, 1024 };
  for(size_t r = 0; r < 7; ++r)
    {
      Filesystem::mkdir((boost::format("%s/%d") % base % res[r]).str());
      for(std::string::iterator i = hex.begin(); i != hex.end(); ++i)
        for(std::string::iterator j = hex.begin(); j != hex.end(); ++j)
          {
            Filesystem::mkdir((boost::format("%s/%d/%c%c") % base % res[r] % *i % *j).str());
          }
    }
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

std::string
Filesystem::realpath(const std::string& pathname)
{
  char* result = ::realpath(pathname.c_str(), NULL);
  std::string res = result;
  free(result);
  
  return res;
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
Filesystem::generate_jpeg_file_list(const std::string& pathname, std::vector<std::string>& file_list)
{
  if (Filesystem::is_directory(pathname))
    {
      std::vector<std::string> dir_list = Filesystem::open_directory(pathname);
      for(std::vector<std::string>::iterator i = dir_list.begin(); i != dir_list.end(); ++i)
        {
          generate_jpeg_file_list(*i, file_list);
        }
    }
  else
    {
      if (Filesystem::has_extension(pathname, ".jpg")  ||
          Filesystem::has_extension(pathname, ".JPG")  ||
          Filesystem::has_extension(pathname, ".jpe")  ||
          Filesystem::has_extension(pathname, ".JPE")  ||
          Filesystem::has_extension(pathname, ".JPEG") ||
          Filesystem::has_extension(pathname, ".jpeg"))
        {
          file_list.push_back("file://" + Filesystem::realpath(pathname));
        }
    }
}

/* EOF */
