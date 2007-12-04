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

#include <assert.h>
#include <iostream>
#include "filesystem.hpp"
#include "jpeg.hpp"
#include "md5.hpp"
#include <fstream>
#include "file_entry_cache.hpp"

SWSurfaceHandle
FileEntry::get_thumbnail(int res)
{
  if (res == 0)
    { // Load original
      // Doesn't now URL!
    }
  else
    { // Load Thumbnail
    }
  
  return SWSurfaceHandle();
}

SWSurfaceHandle
FileEntry::get_original()
{
  return SWSurfaceHandle();
}

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
      
          uint16_t url_len;
          in.read((char*)(&url_len), sizeof(uint16_t));
          char url[url_len];
          in.read(url,  url_len);
          entry.url = url;

          in.read((char*)(&entry.mtime),        sizeof(entry.mtime));
          in.read((char*)(&entry.width),        sizeof(entry.width));
          in.read((char*)(&entry.height),       sizeof(entry.height));

          uint8_t num_thumbnails = 0;
          in.read((char*)(&num_thumbnails), sizeof(num_thumbnails));
          for(int i = 0; i < num_thumbnails; ++i)
            {
              ThumbnailEntry thumb_entry;
              in.read((char*)(&thumb_entry.offset), sizeof(thumb_entry.offset));
              in.read((char*)(&thumb_entry.len),    sizeof(thumb_entry.len));
              entry.thumbnails.push_back(thumb_entry);
            }
          
          // FIXME: Do error checking to avoid adding incomplete entries
          entries[entry.url] = entry;
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

          out.write(entry.url.c_str(),  entry.url.length()+1);
          out.write((char*)(&entry.mtime),        sizeof(entry.mtime));
          out.write((char*)(&entry.width),        sizeof(entry.width));
          out.write((char*)(&entry.height),       sizeof(entry.height));

          assert(entry.thumbnails.size() < 256);
          uint8_t num_thumbnails = entry.thumbnails.size();
          out.write((char*)(&num_thumbnails), sizeof(num_thumbnails));
          for(int i = 0; i < num_thumbnails; ++i)
            {
              out.write((char*)(&entry.thumbnails[i].offset), sizeof(entry.thumbnails[i].offset));
              out.write((char*)(&entry.thumbnails[i].len),    sizeof(entry.thumbnails[i].len));
            }        
        }
    }
}

const FileEntry*
FileEntryCache::get_entry(const std::string& url)
{
  Entries::const_iterator i = entries.find(url);
  if (i == entries.end())
    {
      FileEntry entry;
      try { 
        entry.url = url;
        entry.mtime = Filesystem::get_mtime(url.substr(7));
        JPEG::get_size(url.substr(7), entry.width, entry.height);
      } catch (std::exception& err) {
        std::cout << url << ": " << err.what() << std::endl;
        return 0;
      }
      
      entries[url] = entry;
      return &entries[url];
    }
  else
    {
      return &i->second;
    }
}

/* EOF */
