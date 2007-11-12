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

#ifndef HEADER_WORKSPACE_HPP
#define HEADER_WORKSPACE_HPP

#include <vector>
#include "filesystem.hpp"
#include "griv.hpp"
#include "image.hpp"

inline bool has_suffix(const std::string& str, const std::string& suffix)
{
  if (str.length() >= suffix.length())
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
  else
    return false;
}


class Workspace
{
public:
  std::vector<Image*> images;
  int res;
  
public:
  Workspace()
  {
    res = 16;
  }

  void draw()
  {
    int w = int(sqrt(4 * images.size() / 3));
    for(int i = 0; i < int(images.size()); ++i)
      {
        images[i]->draw((i % w) * res + x_offset + screen->w/2,
                        (i / w) * res + y_offset + screen->h/2,
                        res);
      }
  }

  void add(const std::string& filename)
  {
    // if directory, do recursion
    if (is_directory(filename))
      {
        std::cout << '.' << std::flush;
        std::vector<std::string> dir_list = open_directory(filename);
        for(std::vector<std::string>::iterator i = dir_list.begin(); i != dir_list.end(); ++i)
          {
            add(*i);
          }
      }
    else if (has_suffix(filename, ".jpg") ||
             has_suffix(filename, ".JPG")
             )
      {
        std::string md5 = getxattr(filename);
        if (!md5.empty())
          {
            images.push_back(new Image(md5));
          }
        else
          {
            std::cout << "Ignoring: " << filename << std::endl;
          }
      }
  }

  void zoom_in()
  {
    res *= 2;
    if (res > 1024)
      res = 1024;
    else
      { //300,200 ~ 212, 134 ~ 64, 0
        x_offset *= 2;
        y_offset *= 2;
      }
  }

  void zoom_out()
  {
    res /= 2;
    if (res < 16)
      res = 16;
    else
      {
        x_offset /= 2;
        y_offset /= 2;
      }
  }
};

#endif

/* EOF */
