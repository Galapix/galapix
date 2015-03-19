/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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
#include <uitest/uitest.hpp>

#include "sqlite/connection.hpp"
#include "database/tables/file_table.hpp"
#include "database/statements/file_entry_get_all.hpp"
#include "database/statements/file_entry_store.hpp"
#include "database/statements/file_entry_delete.hpp"
#include "database/statements/file_entry_get_by_file_id.hpp"
#include "database/statements/file_entry_get_by_pattern.hpp"

UITEST(FileTable, test, "",
       "FileTable test")
{
  SQLiteConnection db("");
  FileTable file_table(db);
  FileEntryGetAll file_entry_get_all(db);
  FileEntryStore file_entry_store(db);
  FileEntryDelete file_entry_delete(db);
  FileEntryGetByFileId file_entry_get_by_file_id(db);
  FileEntryGetByPattern file_entry_get_by_pattern(db);

  {
    std::cout << "store test" << std::endl;
    std::cout << "inserted " << file_entry_store("/tmp/foo/bar", 34234234) << std::endl;
    std::cout << "inserted " << file_entry_store("/tmp/bar/blub", 98098) << std::endl;
    std::cout << "inserted " << file_entry_store("/tmp/foobar/num", -1) << std::endl;
    std::cout << "store test: done" << std::endl;
  }

  {
    std::cout << "get test" << std::endl;
    std::vector<FileEntry> entries;
    file_entry_get_all(entries);
    std::cout << "---- snip ----" << std::endl;
    for(const auto& entry : entries)
    {
      std::cout << entry.get_path() << " " << entry.get_mtime() << std::endl;
    }
    std::cout << "---- snip ----" << std::endl;
    std::cout << "get test: done" << std::endl;
  }


  {
    std::cout << "get pattern test" << std::endl;
    std::vector<FileEntry> entries;
    file_entry_get_by_pattern("/tmp/foo*", entries);
    std::cout << "---- snip ----" << std::endl;
    for(const auto& entry : entries)
    {
      std::cout << entry.get_path() << " " << entry.get_mtime() << std::endl;
    }
    std::cout << "---- snip ----" << std::endl;
    std::cout << "get pattern test: done" << std::endl;
  }

  {
    std::cout << "delete test" << std::endl;
    std::vector<FileEntry> entries;
    file_entry_get_all(entries);
    for(const auto& entry : entries)
    {
      file_entry_delete(entry.get_id());
    }
    std::cout << "delete test: done" << std::endl;
  }

  {
    std::cout << "get test again" << std::endl;
    std::vector<FileEntry> entries;
    file_entry_get_all(entries);
    std::cout << "---- snip ----" << std::endl;
    for(const auto& entry : entries)
    {
      std::cout << entry.get_path() << " " << entry.get_mtime() << std::endl;
    }
    std::cout << "---- snip ----" << std::endl;
  }
}

/* EOF */
