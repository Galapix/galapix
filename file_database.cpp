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
#include <sstream>
#include <stdexcept>

#include <assert.h>
#include "jpeg.hpp"
#include "md5.hpp"
#include "filesystem.hpp"
#include "file_database.hpp"

std::ostream& operator<<(std::ostream& os, const FileEntry& entry)
{
  return os << "filename: " << entry.filename << " size: "<< entry.width << "x" << entry.height;
}

FileDatabase::FileDatabase(SQLiteConnection* db)
  : db(db),
    store_stmt(db),
    get_by_filename_stmt(db),
    get_by_file_id_stmt(db)
{
  db->exec("CREATE TABLE IF NOT EXISTS files ("
           "fileid    INTEGER PRIMARY KEY AUTOINCREMENT,"
           "filename  TEXT UNIQUE, "
           "md5       TEXT, "
           "filesize  INTEGER, "
           "width     INTEGER, "
           "height    INTEGER, "
           "mtime     INTEGER);");

  //db->exec("CREATE UNIQUE INDEX IF NOT EXISTS files_index ON files ( filename, md5 );");

  store_stmt.prepare("INSERT INTO files (filename, md5, filesize, width, height, mtime) VALUES (?1, ?2, ?3, ?4, ?5, ?6);");
  get_by_filename_stmt.prepare("SELECT * FROM files WHERE filename = ?1;");
  get_by_file_id_stmt.prepare("SELECT * FROM files WHERE rowid = ?1;");
}
 
FileDatabase::~FileDatabase()
{

}
 
int
FileDatabase::store_file_entry(FileEntry& entry)
{
  assert(entry.fileid == -1);

  store_stmt.bind_text(1, entry.filename);
  store_stmt.bind_text(2, entry.md5);
  store_stmt.bind_int (3, entry.filesize); // filesize
  store_stmt.bind_int (4, entry.width);    // width
  store_stmt.bind_int (5, entry.height);   // height
  store_stmt.bind_int (6, entry.mtime);        // mtime

  store_stmt.execute();
  
  entry.fileid = sqlite3_last_insert_rowid(db->get_db());

  return entry.fileid;
}

bool
FileDatabase::get_file_entry(const std::string& filename, FileEntry& entry)
{
  //std::cout << "Trying to retrieve: " << filename << std::endl;
  get_by_filename_stmt.bind_text(1, filename);
  SQLiteReader reader = get_by_filename_stmt.execute_query();

  if (reader.next())
    {
      if (0)
        std::cout << "Row: " 
                  << reader.get_column_name(0) << " "
                  << reader.get_text(0)
                  << std::endl;

      entry.fileid   = reader.get_int (0);
      entry.filename = reader.get_text(1);
      entry.md5      = reader.get_text(2);
      entry.filesize = reader.get_int (3);
      entry.width    = reader.get_int (4);
      entry.height   = reader.get_int (5);

      return true;
    }
  else
    {
      entry.fileid   = -1;
      entry.filename = filename;
      entry.md5      = MD5::md5_file(filename);
      entry.filesize = Filesystem::get_size(filename);
      entry.mtime    = Filesystem::get_mtime(filename);
      
      entry.width    = -1;
      entry.height   = -1;
      
      // Doesn't work due to linking issues
      // JPEG::get_size(entry.filename, entry.width, entry.height);

      store_file_entry(entry);
      
      return true;
    }
}

bool
FileDatabase::get_file_entry(uint32_t file_id, FileEntry& entry)
{
  get_by_file_id_stmt.bind_int(1, file_id);
  get_by_file_id_stmt.execute_query();

  // If nothing is found, query the file system and store the results

  return false;
}

/* EOF */
