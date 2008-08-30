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
#include "filesystem.hpp"
#include "software_surface.hpp"
#include "file_entry.hpp"
#include "file_database.hpp"

FileDatabase::FileDatabase(SQLiteConnection* db)
  : db(db),
    store_stmt(db),
    get_by_filename_stmt(db),
    get_all_stmt(db),
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

  db->exec("CREATE UNIQUE INDEX IF NOT EXISTS files_index ON files ( filename );");

  store_stmt.prepare("INSERT INTO files (filename, md5, filesize, width, height, mtime) VALUES (?1, ?2, ?3, ?4, ?5, ?6);");
  get_by_filename_stmt.prepare("SELECT * FROM files WHERE filename = ?1;");
  get_by_file_id_stmt.prepare("SELECT * FROM files WHERE rowid = ?1;");
  get_all_stmt.prepare("SELECT * FROM files ORDER BY filename");
}
 
FileDatabase::~FileDatabase()
{

}
 
int
FileDatabase::store_file_entry(FileEntry& entry)
{
  assert(entry.fileid == -1);

  store_stmt.bind_text(1, entry.filename);
  if (entry.md5.empty())
    store_stmt.bind_null(2);
  else
    store_stmt.bind_text(2, entry.md5);
  store_stmt.bind_int (3, entry.filesize); 
  store_stmt.bind_int (4, entry.size.width); 
  store_stmt.bind_int (5, entry.size.height);
  store_stmt.bind_int (6, entry.mtime); 

  store_stmt.execute();
  
  entry.fileid = sqlite3_last_insert_rowid(db->get_db());

  return entry.fileid;
}

bool
FileDatabase::get_file_entry(const std::string& filename, FileEntry* entry)
{
  get_by_filename_stmt.bind_text(1, filename);
  SQLiteReader reader = get_by_filename_stmt.execute_query();

  if (reader.next())
    {
      if (0)
        std::cout << "Row: " 
                  << reader.get_column_name(0) << " "
                  << reader.get_text(0)
                  << std::endl;

      entry->fileid      = reader.get_int (0);
      entry->filename    = reader.get_text(1);
      entry->md5         = reader.get_text(2);
      entry->filesize    = reader.get_int (3);
      entry->size.width  = reader.get_int (4);
      entry->size.height = reader.get_int (5);

      return true;
    }
  else
    {
      entry->fileid   = -1;
      entry->filename = filename;
      entry->filesize = Filesystem::get_size(filename);
      entry->mtime    = Filesystem::get_mtime(filename);
      
      entry->size = Size(-1, -1);
      
      if (JPEG::get_size(entry->filename, entry->size))
        {
          store_file_entry(*entry);
          return true;
        }
      else
        {
          return false;
        }
    }
}

void
FileDatabase::get_file_entries(std::vector<FileEntry>& entries)
{
  SQLiteReader reader = get_all_stmt.execute_query();

  while (reader.next())  
    {
      FileEntry entry;
      entry.fileid      = reader.get_int (0);
      entry.filename    = reader.get_text(1);
      entry.md5         = reader.get_text(2);
      entry.filesize    = reader.get_int (3);
      entry.size.width  = reader.get_int (4);
      entry.size.height = reader.get_int (5);
      entry.mtime       = reader.get_int (6);

      entries.push_back(entry);
    }
}

void
FileDatabase::delete_file_entry(uint32_t fileid)
{
  // DELETE FROM files WHERE fileid = ?fileid
}

void
FileDatabase::update_file_entry(FileEntry& entry)
{
  // UPDATE files SET mtime = ?entry.get_mtime() WHERE fileid = ?entry.fileid
}

void
FileDatabase::check()
{
  std::vector<FileEntry> entries;
  get_file_entries(entries);

  std::cout << "Checking File Existance:" << std::endl;
  for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
    {
      if (!Filesystem::exist(i->filename))
        {
          std::cout << i->filename << ": does not exist" << std::endl;
        }
      else
        {
          std::cout << i->filename << ": ok" << std::endl;
        }
    } 

  /* FIXME: Do magic to detect duplicate file entries and other potential damage to the database (are missing tiles an error?) 

    SQLiteStatement duplicates_stmt(db);
    duplicates_stmt.prepare("SELECT * IN files (filename, md5, filesize, width, height, mtime) VALUES (?1, ?2, ?3, ?4, ?5, ?6);");
    SELECT filename,
    COUNT(filename) AS NumOccurrences
    FROM files
    GROUP BY filename
    HAVING ( COUNT(filename) > 1 )
    
    SQLiteReader reader = duplicates_stmt.execute_query();
  */
}

/* EOF */
