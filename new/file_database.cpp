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
#include "file_database.hpp"

FileDatabase::FileDatabase(sqlite3* db)
  : db(db)
{
  if (sqlite3_prepare_v2(db,
                         "INSERT OR REPLACE INTO files (filename, md5, filesize, width, height, mtime)"
                         "           VALUES (      ?1,  ?2,       ?3,    ?4,     ?5,    ?6);", -1, &store_stmt,  0)
      != SQLITE_OK)
    {
      fprintf(stderr, "Something went wrong2: %s\n", sqlite3_errmsg(db));
    }
  
  if (sqlite3_prepare_v2(db, "SELECT * FROM files WHERE filename = ?1", -1, &get_by_file_id_stmt,  0))
    {
      fprintf(stderr, "Something went wrong2: %s\n", sqlite3_errmsg(db));      
    }

  if (sqlite3_prepare_v2(db, "SELECT * FROM files WHERE rowid = ?1",  -1, &get_by_file_id_stmt,  0))
    {
      fprintf(stderr, "Something went wrong2: %s\n", sqlite3_errmsg(db));      
    }

}
  
int
FileDatabase::store_file_entry(const std::string& filename, const std::string& md5, size_t filesize, int width, int height, int mtime)
{
  sqlite3_bind_blob(store_stmt, 1, filename.c_str(), filename.size(), SQLITE_STATIC);
  sqlite3_bind_text(store_stmt, 2, md5.c_str(), -1, SQLITE_STATIC); 
  sqlite3_bind_int (store_stmt, 3, filesize); // filesize
  sqlite3_bind_int (store_stmt, 4, width);    // width
  sqlite3_bind_int (store_stmt, 5, height);   // height
  sqlite3_bind_int (store_stmt, 6, 0);        // mtime

  // execute
  int rc = sqlite3_step(store_stmt);
  if (rc != SQLITE_DONE)
    {
      std::cout << "Something went wrong: " << sqlite3_errmsg(db) << std::endl;
    }
  else
    {
      std::cout << "added " << filename << std::endl;
    }

  sqlite3_reset(store_stmt);
  sqlite3_clear_bindings(store_stmt);  

  return 0; // row_id
}

FileEntry
FileDatabase::get_file_entry(const std::string& filename)
{
  sqlite3_bind_text(get_by_filename_stmt, 1, filename.c_str(), -1, SQLITE_STATIC);

  // execute
  int rc = sqlite3_step(get_by_filename_stmt);
  if (rc != SQLITE_DONE)
    {
      std::cout << "Something went wrong: " << sqlite3_errmsg(db) << std::endl;
    }
  else
    {
      std::cout << "added " << filename << std::endl;
    }

  sqlite3_reset(get_by_filename_stmt);
  sqlite3_clear_bindings(get_by_filename_stmt);  
  return FileEntry();
}

FileEntry
FileDatabase::get_file_entry(uint32_t file_id)
{
  sqlite3_bind_int(get_by_file_id_stmt, 1, file_id);
  return FileEntry();
}

/* EOF */
