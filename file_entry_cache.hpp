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

#ifndef HEADER_CACHE_HPP
#define HEADER_CACHE_HPP

#include <string>
#include <map>
#include "software_surface.hpp"

struct ThumbnailEntry
{
  uint64_t offset;
  uint32_t len;
};

struct FileEntry 
{
  std::string url;    // FIXME: Could save raw MD5 instead of string version
  uint32_t    mtime;  // mtime of the file
  int         width;  // image width
  int         height; // image height
  
  /** 
      thumbnails[0] -> 16x16
      thumbnails[1] -> 32x32
      ...
      thumbnails[N] -> (16 + 2^N)x(16 + 2^N)
   */
  std::vector<ThumbnailEntry> thumbnails;

  SWSurfaceHandle get_thumbnail(int res);
  SWSurfaceHandle get_original();
};

/** */
class FileEntryCache
{

private:
  typedef std::map<std::string, FileEntry> Entries;
  Entries entries;

public:
  FileEntryCache(const std::string& filename);
  
  void save(const std::string& filename) const;

  /** returns NULL if the FileEntry isn't obtainable (file missing,
      etc.) */
  const FileEntry* get_entry(const std::string& url);
};

#endif

/* EOF */
