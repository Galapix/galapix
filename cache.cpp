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

#include <fstream>
#include "cache.hpp"

Cache::Cache(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

  while(!in.eof())
    {
      FileEntry entry;
      
      in.read(entry.filename_md5,  sizeof(char) * 33);
      in.read(entry.md5,           sizeof(char) * 33);
      in.read((char*)(&entry.mtime),        sizeof(unsigned int));
      in.read((char*)(&entry.thumbnail_id), sizeof(unsigned int));
      in.read((char*)(&entry.width),        sizeof(unsigned int));
      in.read((char*)(&entry.height),       sizeof(unsigned int));
      
      entries[entry.filename_md5] = entry;
    }
}

const FileEntry*
Cache::get_entry(const std::string& url) const
{
  Entries::const_iterator i = entries.find(url);
  if (i == entries.end())
    {
      return 0;
    }
  else
    {
      return &i->second;
    }
}

/* EOF */
