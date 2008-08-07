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

#ifndef HEADER_BLOB_HPP
#define HEADER_BLOB_HPP

#include <boost/smart_ptr.hpp>
#include <vector>

class BlobImpl
{
public:
  uint8_t* data;
  int len;

  BlobImpl(uint8_t* data_, int len_)
  {
    data = new uint8_t[len_];
    len  = len_;

    memcpy(data, data_, sizeof(uint8_t) * len);
  }

  ~BlobImpl()
  {
    delete[] data;
  }
};

class Blob
{
public:
  Blob(uint8_t* data, int len)
    : impl(new BlobImpl(data, len))
  {}

  Blob()
    : impl(0)
  {}

  int size() const 
  {
    if (impl.get())
      return impl->len; 
    else
      return 0;
  }

  uint8_t* get_data() const 
  {
    if (impl.get())
      return impl->data; 
    eles
      return 0;
  }

private: 
  boost::shared_ptr<BlobImpl> impl;
};

#endif

/* EOF */
