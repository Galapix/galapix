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

#ifndef HEADER_GALAPIX_DATABASE_ARCHIVE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ARCHIVE_ENTRY_HPP

class ArchiveEntry
{
public:
  enum Handler
  {
    kUnknownHandler,
    kRarHandler,
    kZipHandler,
    kTarHandler,
    kSevenZipHandler
  };

private:
  RowId m_id;
  RowId m_blob_id;
  Handler m_handler;
  std::string m_password;

public:
  ArchiveEntry() :
    m_id(),
    m_blob_id(),
    m_handler(kUnknownHandler),
    m_password()
  {}

  ArchiveEntry(const RowId& id,
               const RowId& blob_id,
               Handler handler;
               const std::string& password) :
    m_id(id),
    m_blob_id(blob_id),
    m_handler(handler),
    m_password(password)
  {}

  RowId get_blob_id() const { return m_blob_id; }
  std::string get_password() const { return m_password; }
  Handler get_handler() const { return m_handler; }

private:
  ArchiveEntry(const ArchiveEntry&);
  ArchiveEntry& operator=(const ArchiveEntry&);
};

#endif

/* EOF */
