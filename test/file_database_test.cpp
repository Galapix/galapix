/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#include "sqlite/connection.hpp"
#include "database/file_database.hpp"
#include "util/filesystem.hpp"
#include "util/sha1.hpp"

int main(int argc, char** argv)
{
  SQLiteConnection db("/tmp/database.sqlite3");
  FileDatabase filedb(db);

  for(int i = 1; i < argc; ++i)
  {
    URL url   = URL::from_filename(argv[i]);
    int size  = url.get_size();
    int mtime = url.get_mtime();

    FileEntry file_entry;
    if (!filedb.get_file_entry(url, file_entry))
    { 
      SHA1 sha1 = SHA1::from_file(url.get_stdio_name());
      file_entry = filedb.store_file_entry(url, sha1, size, mtime, FileEntry::kUnknownHandler, RowId());
      std::cout << "stored: " << file_entry << std::endl;
    }
    else
    {
      SHA1 sha1 = SHA1::from_file(url.get_stdio_name());
      if (size  != file_entry.get_blob_entry().get_size() ||
          mtime != file_entry.get_mtime())
      { 
        file_entry = filedb.store_file_entry(url, sha1, size, mtime, FileEntry::kUnknownHandler, RowId());
        std::cout << "replaced: " << file_entry << std::endl;
      }
      else
      {
        // file already in the database
        std::cout << "ignored: " << file_entry << std::endl;
      }
    }
  }

  std::cout << "\nAll Entries: " << std::endl;
  std::vector<FileEntry> entries;
  filedb.get_file_entries(entries);
  for(auto it = entries.begin(); it != entries.end(); ++it)
  {
    std::cout << *it << std::endl;
  }
  
  return 0;
}

/* EOF */
