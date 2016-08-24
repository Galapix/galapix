/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "util/md5.hpp"

// clang doesn't define _Bool in C++ mode, but mhash needs it
#ifndef _Bool
#  define _Bool bool
#  include <mhash.h>
#  undef _Bool
#else
#  include <mhash.h>
#endif

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "util/raise_exception.hpp"

std::string
MD5::from_string(const std::string& str)
{
  unsigned char hash[16]; /* enough size for MD5 */
  MHASH td = mhash_init(MHASH_MD5);
  if (td == MHASH_FAILED)
  {
    raise_runtime_error("Failed to init MHash");
  }
  else
  {
    mhash(td, str.c_str(), static_cast<mutils_word32>(str.length()));

    mhash_deinit(td, hash);

    // Convert to string representation
    std::ostringstream out;
    for (int i = 0; i < 16; i++)
      out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);

    return out.str();
  }
}

std::string
MD5::from_file(const std::string& filename)
{
  unsigned char hash[16]; /* enough size for MD5 */
  MHASH td = mhash_init(MHASH_MD5);
  if (td == MHASH_FAILED)
  {
    raise_runtime_error("Failed to init MHash");
  }
  else
  {
    const unsigned int buf_size = 32768;
    char buf[buf_size];
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
      raise_runtime_error("Couldn't open file " + filename);
    }
    else
    {

      while(!in.eof())
      {
        in.read(buf, buf_size);
        mhash(td, buf, static_cast<mutils_word32>(in.gcount()));
      }

      in.close();

      mhash_deinit(td, hash);

      // Convert to string representation
      std::ostringstream out;
      for (int i = 0; i < 16; i++)
        out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);

      return out.str();
    }
  }
}

/* EOF */
