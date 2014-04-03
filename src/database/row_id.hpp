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

#ifndef HEADER_GALAPIX_DATABASE_ROW_ID_HPP
#define HEADER_GALAPIX_DATABASE_ROW_ID_HPP

#include <stdint.h>
#include <assert.h>
#include <iosfwd>

class RowId
{
private:
  int64_t m_id;

public:
  RowId() : m_id(0) {}
  RowId(int64_t id) : m_id(id) {}

  int64_t get_id() const
  {
    assert(m_id != 0);
    return m_id;
  }

  bool operator==(const RowId& rhs) const
  {
    if (m_id == 0 || rhs.m_id == 0)
    {
      return false;
    }
    else
    {
      return m_id == rhs.m_id;
    }
  }

  explicit operator bool() const
  {
    return m_id != 0;
  }
};

std::ostream& operator<<(std::ostream& s, const RowId& id);

#endif

/* EOF */
