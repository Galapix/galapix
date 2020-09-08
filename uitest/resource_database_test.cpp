/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmail.com>
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
#include "database/resource_database.hpp"
#include "util/filesystem.hpp"
#include "util/sha1.hpp"

UITEST(ResourceDatabase, test, "")
{
  SQLiteConnection db(""); //("/tmp/resource_database_test.sqlite3");
  ResourceDatabase res_db(db);

  res_db.store_file_info(FileInfo("/tmp/hello_world.txt", 123, galapix::SHA1::from_mem("hello world"), 98787));
  res_db.store_file_info(FileInfo("/tmp/foobar.txt", 456, galapix::SHA1::from_mem("foobar"), 1234578));
  res_db.store_file_info(FileInfo("/tmp/barfoo.txt", 789, galapix::SHA1::from_mem("barfoo"), 1234578));

  for(auto& path : { "/tmp/foobar.txt", "/tmp/barfoo.txt", "/tmp/unknown.txt", "/tmp/hello_world.txt",  })
  {
    std::optional<FileInfo> file_info = res_db.get_file_info(path);
    if (file_info)
    {
      std::cout << file_info->get_path() << '\n'
                << "  mtime : " << file_info->get_mtime() << '\n'
                << "  sha1  : " << file_info->get_sha1().str() << '\n'
                << "  size  : " << file_info->get_size() << '\n'
                << std::endl;
    }
    else
    {
      std::cout << path << ": not found\n" << std::endl;
    }
  }


  if ((false))
  {
    res_db.store_file_entry("/tmp/foo/bar.txt", 23445);
    res_db.store_file_entry("/tmp/foo/bam.txt", 2989);
    res_db.store_file_entry("/tmp/bar/foo.txt", 298998);

    std::vector<FileEntry> entries;
    res_db.get_file_entries(entries);
    std::cout << "got " << entries.size() << " entries" << std::endl;
  }
}

/* EOF */
