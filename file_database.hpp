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

#ifndef HEADER_FILE_DATABASE_HPP
#define HEADER_FILE_DATABASE_HPP

#include <sqlite3.h>
#include <string>
#include "sqlite.hpp"

struct FileEntry 
{
  std::string filename; // 
  uint32_t    mtime;    // mtime of the file
  int         width;    // image width
  int         height;   // image height
};

std::ostream& operator<<(std::ostream& os, const FileEntry& entry);

/** */
class FileDatabase
{
private:
  SQLiteConnection* db;
  SQLiteStatement store_stmt;
  SQLiteStatement get_by_filename_stmt;
  SQLiteStatement get_by_file_id_stmt;

  int store_file_entry(const std::string& filename, const std::string& md5, size_t filesize, int width, int height, int mtime);
 
public:
  FileDatabase(SQLiteConnection* db);
  ~FileDatabase();
  
  FileEntry get_file_entry(const std::string& filename);
  FileEntry get_file_entry(uint32_t file_id);

private:
  FileDatabase (const FileDatabase&);
  FileDatabase& operator= (const FileDatabase&);
};

#endif

/* EOF */
