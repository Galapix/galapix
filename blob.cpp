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

#include <fstream>
#include "blob.hpp"

class BlobImpl
{
public:
  uint8_t* data;
  int      len;

  BlobImpl(const void* data_, int len_)
  {
    data = new uint8_t[len_];
    len  = len_;

    memcpy(data, data_, len);
  }

  BlobImpl(const std::vector<uint8_t>& data_in)
  {
    data = new uint8_t[data_in.size()];
    len  = data_in.size();

    memcpy(data, &*data_in.begin(), len);
  }

  ~BlobImpl()
  {
    delete[] data;
  }
};

Blob::Blob(const std::vector<uint8_t>& data)
  : impl(new BlobImpl(data))
{}

Blob::Blob(const void* data, int len)
  : impl(new BlobImpl(data, len))
{}

Blob::Blob()
{}

int
Blob::size() const 
{
  if (impl.get())
    return impl->len; 
  else
    return 0;
}

uint8_t* 
Blob::get_data() const 
{
  if (impl.get())
    return impl->data; 
  else
    return 0;
}

void
Blob::write_to_file(const std::string& filename)
{
  std::ofstream out(filename.c_str(), std::ios::binary);
  out.write(reinterpret_cast<char*>(impl->data), impl->len);
}

/* EOF */
