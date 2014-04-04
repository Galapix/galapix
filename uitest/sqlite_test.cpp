/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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
#include "sqlite/statement.hpp"

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    std::cerr << "Usage: " << argv[0] << " SQLITEDATABASE TABLE" << std::endl;
  }
  else
  {
    while(true)
      try 
      {
        SQLiteConnection db(argv[1]);

        SQLiteStatement stmt(db, std::string("select * from ") + argv[2] + ";"); // FIXME: totaly not safe
        SQLiteReader reader = stmt.execute_query();

        const int columns = reader.get_column_count();
        for(int i = 0; i < columns; ++i)
        {
          std::cout << "column: " << i << ": " << reader.get_column_name(i) << std::endl;
        }

        int rows = 0;
        while(reader.next())
        {
          std::cout << "row: " << rows << " ";
          for(int i = 0; i < columns; ++i)
          {
            switch(reader.get_type(i))
            {
              case SQLITE_INTEGER:
                std::cout << reader.get_int(i);
                break;

              case SQLITE_FLOAT:
                std::cout << reader.get_float(i);
                break;

              case SQLITE_BLOB:
                {
                  auto blob = reader.get_blob(i);
                  std::cout << "blob:" << blob->size();
                }
                break;

              case SQLITE_NULL:
                std::cout << "(null)";
                break;

              case SQLITE_TEXT:
                std::cout << reader.get_text(i);
                break;

              default:
                std::cout << "(unknown)";
                break;
            }
        
            std::cout << " | ";
          }
          std::cout << std::endl;
          rows += 1;
        }
        std::cout << "rows: " << rows << std::endl;
      }
      catch(const std::exception& err)
      {
        std::cerr << "error: " << err.what() << std::endl;
      }
  }

  return 0;
}

/* EOF */
