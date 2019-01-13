/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_ENTRIES_RESOURCE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ENTRIES_RESOURCE_ENTRY_HPP

#include "database/row_id.hpp"

class ResourceEntry
{
public:
  enum Type {};
  enum Handler
  {
    kUnknownHandler,
    kJPEGHandler,
    kPNGHandler,
    kRSVGHandler,
    kKRAHandler,
    kUFRawHandler,
    kVidThumbHandler,
    kDDSHandler,
    kImagemagickHandler
  };

public:
  ResourceEntry();

  Type    get_type() const { return m_type; }
  Handler get_handler() const { return m_handler; }
  RowId   get_blob_id() const { return m_blob_id; }
  std::string get_arguments() const { return m_arguments; }

private:
  Type m_type;
  Handler m_handler;
  RowId m_blob_id;
  std::string m_arguments;
};

#endif

/* EOF */
