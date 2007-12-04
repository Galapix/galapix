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

#include <iostream>
#include "filesystem.hpp"
#include "jpeg.hpp"
#include "md5.hpp"
#include <fstream>
#include "file_entry_cache.hpp"

FileEntryCache::FileEntryCache(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
  if (!in)
    {
      // do nothing, since empty cache is ok
    }
  else
    {
      while(!in.eof())
        {
          FileEntry entry;
      
          in.read(entry.url_md5,  sizeof(char) * 33);
          in.read((char*)(&entry.mtime),        sizeof(unsigned int));
          in.read((char*)(&entry.thumbnail_id), sizeof(unsigned int));
          in.read((char*)(&entry.width),        sizeof(unsigned int));
          in.read((char*)(&entry.height),       sizeof(unsigned int));

          // FIXME: Do error checking to avoid adding incomplete entries
          entries[entry.url_md5] = entry;
        }
    }
}

void
FileEntryCache::save(const std::string& filename) const
{
  std::ofstream out(filename.c_str(), std::ios::out | std::ios::binary);

  if (!out)
    {
      std::cout << "Couldn't save cache" << std::endl;
    }
  else
    {
      for(Entries::const_iterator i = entries.begin();
          i != entries.end(); ++i)
        {
          const FileEntry& entry = i->second;
          out.write(entry.url_md5,  sizeof(char) * 33);
          out.write((char*)(&entry.mtime),        sizeof(unsigned int));
          out.write((char*)(&entry.thumbnail_id), sizeof(unsigned int));
          out.write((char*)(&entry.width),        sizeof(unsigned int));
          out.write((char*)(&entry.height),       sizeof(unsigned int));
        }
    }
}

const FileEntry*
FileEntryCache::get_entry(const std::string& url)
{
  std::string url_md5 = MD5::md5_string(url);

  Entries::const_iterator i = entries.find(url_md5);
  if (i == entries.end())
    {
      FileEntry entry;
      try { 
        strcpy(entry.url_md5, url_md5.c_str());
        entry.mtime = Filesystem::get_mtime(url.substr(7));
        entry.thumbnail_id = 0;
        JPEG::get_size(url.substr(7), entry.width, entry.height);
      } catch (std::exception& err) {
        std::cout << url << ": " << err.what() << std::endl;
        return 0;
      }
      
      entries[url_md5] = entry;

      return &entries[url_md5];
    }
  else
    {
      return &i->second;
    }
}

/* EOF */
