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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_IMAGE_INFO_GET_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_IMAGE_INFO_GET_HPP

class ImageInfoGet
{
private:
  SQLiteStatement m_stmt;

public:
  ImageInfoGet(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  image.id, image.width, image.height\n"
           "FROM\n"
           "  image\n"
           "WHERE\n"
           "       ");
  {}

  boost::optional<ImageInfo> operator()(const ResourceInfo& resource_info)
  {
    m_stmt.bind_text(1, url);
    SQLiteReader reader = m_stmt.execute_query();

    if (reader.next())
    {
      return ImageInfo(RowId(reader.get_int64(0)),
                       reader.get_int(1),
                       reader.get_int(2));
    }
    else
    {
      return boost::optional<ImageInfo>();
    }
  }

private:
  ImageInfoGet(const ImageInfoGet&) = delete;
  ImageInfoGet& operator=(const ImageInfoGet&) = delete;
};

#endif

/* EOF */
