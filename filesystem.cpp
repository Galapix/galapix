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

#include <stdexcept>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <attr/xattr.h>

#include "filesystem.hpp"

bool is_directory(const std::string& pathname)
{
  struct stat buf;
  stat(pathname.c_str(), &buf);
  return S_ISDIR(buf.st_mode);
}

std::vector<std::string>
open_directory(const std::string& pathname)
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

std::string getxattr(const std::string& pathname)
{
  char buf[2048];
  int len;
  if ((len = getxattr (pathname.c_str(), "user.griv.md5", buf, 2048)) < 0)
    {
      if (errno == ENOATTR)
        return "";
      else
        throw std::runtime_error("Couldn't get xattr for " + pathname);
    }

  return std::string(buf, len);
}

/* EOF */
