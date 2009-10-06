/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
//#include <sstream>
//#include <stdexcept>
//#include <assert.h>

#include "file_database.hpp"
#include "file_entry.hpp"
#include "software_surface.hpp"
#include "util/filesystem.hpp"
//#include "math/math.hpp"
//#include "tile_entry.hpp"

FileDatabase::FileDatabase(SQLiteConnection* db_)
  : db(db_),
    store_stmt(db),
    get_by_url_stmt(db),
    get_all_stmt(db),
    get_by_pattern_stmt(db),
    get_by_file_id_stmt(db)
{
  db->exec("CREATE TABLE IF NOT EXISTS files ("
           "fileid    INTEGER PRIMARY KEY AUTOINCREMENT,"
           "url       TEXT UNIQUE, "
           "size      INTEGER, "
           "mtime     INTEGER, "
           
           "width     INTEGER, "
           "height    INTEGER);");

  db->exec("CREATE UNIQUE INDEX IF NOT EXISTS files_index ON files ( url );");

  // FIXME: Does replace change the rowid?
  store_stmt.prepare("INSERT OR REPLACE INTO files (url, size, mtime, width, height) VALUES (?1, ?2, ?3, ?4, ?5);");

  get_by_url_stmt.prepare("SELECT * FROM files WHERE url = ?1;");
  get_by_file_id_stmt.prepare("SELECT * FROM files WHERE fileid = ?1;");
  get_by_pattern_stmt.prepare("SELECT * FROM files WHERE url GLOB ?1;");
  get_all_stmt.prepare("SELECT * FROM files");
}
 
FileDatabase::~FileDatabase()
{

}

FileEntry
FileDatabase::store_file_entry(const FileEntry& entry)
{
  return store_file_entry(entry.get_url(), entry.get_image_size());
}
 
FileEntry
FileDatabase::store_file_entry(const URL& url,
                               const Size& size)
{
  FileEntry file_entry(0, url, url.get_size(), url.get_mtime(), size.width, size.height);

  store_stmt.bind_text(1, file_entry.get_url().str());
  store_stmt.bind_int (2, file_entry.get_size());
  store_stmt.bind_int (3, file_entry.get_mtime());
  store_stmt.bind_int (4, file_entry.get_width());
  store_stmt.bind_int (5, file_entry.get_height());

  store_stmt.execute();
  
  file_entry.set_fileid(sqlite3_last_insert_rowid(db->get_db()));

  return file_entry;
}

FileEntry
FileDatabase::get_file_entry(const URL& url)
{
  get_by_url_stmt.bind_text(1, url.str());
  SQLiteReader reader = get_by_url_stmt.execute_query();

  if (reader.next())
    {
      return FileEntry(reader.get_int (0),  // fileid
                       URL::from_string(reader.get_text(1)),  // url
                       reader.get_int(2),
                       reader.get_int(3),
                       reader.get_int (4),  // width
                       reader.get_int (5)); // height
    }
  else
    {
      Size size;
      if (SoftwareSurface::get_size(url, size))
        {
          return store_file_entry(url, size);
        }
      else
        {
          return FileEntry();
        }
    }
}

void
FileDatabase::get_file_entries(std::vector<FileEntry>& entries, const std::string& pattern)
{
  get_by_pattern_stmt.bind_text(1, pattern);
  SQLiteReader reader = get_by_pattern_stmt.execute_query();

  while (reader.next())  
    {
      // FIXME: Use macro definitions instead of numeric constants
      FileEntry entry(reader.get_int(0),  // fileid
                      URL::from_string(reader.get_text(1)),  // url
                      reader.get_int(2),
                      reader.get_int(3),
                      reader.get_int(4),  // width
                      reader.get_int(5)); // height
      entries.push_back(entry);
    } 
}

void
FileDatabase::get_file_entries(std::vector<FileEntry>& entries)
{
  SQLiteReader reader = get_all_stmt.execute_query();

  while (reader.next())  
    {
      // FIXME: Use macro definitions instead of numeric constants
      FileEntry entry(reader.get_int(0),  // fileid
                      URL::from_string(reader.get_text(1)),  // url
                      reader.get_int(2),
                      reader.get_int(3),
                      reader.get_int(4),  // width
                      reader.get_int(5)); // height
      entries.push_back(entry);
    }
}

void
FileDatabase::update_file_entry(FileEntry& /*entry*/)
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
      if (!Filesystem::exist(i->get_url().get_stdio_name()))
        {
          std::cout << i->get_url() << ": does not exist" << std::endl;
        }
      else
        {
          std::cout << i->get_url() << ": ok" << std::endl;
        }
    }
}

/* EOF */
