#if 0
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

#include <mhash.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "util/md5.hpp"

std::string
MD5::md5_string(const std::string& str)
{
  unsigned char hash[16]; /* enough size for MD5 */
  MHASH td = mhash_init(MHASH_MD5);
  if (td == MHASH_FAILED)
    throw std::runtime_error("Failed to init MHash");
  
  mhash(td, str.c_str(), str.length());
  
  mhash_deinit(td, hash);

  // Convert to string representation
  std::ostringstream out;
  for (int i = 0; i < 16; i++) 
    out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);

  return out.str();
}

std::string
MD5::md5_file(const std::string& filename)
{
  unsigned char hash[16]; /* enough size for MD5 */
  MHASH td = mhash_init(MHASH_MD5);
  if (td == MHASH_FAILED) 
    throw std::runtime_error("Failed to init MHash");
  
  const unsigned int buf_size = 32768;
  char buf[buf_size];
  std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary); 
  if (!in)
    throw std::runtime_error("MD5: Couldn't open file " + filename);
    
  while(!in.eof())
  {
    in.read(buf, buf_size);
    mhash(td, buf, in.gcount());
  }

  in.close();
    
  mhash_deinit(td, hash);

  // Convert to string representation
  std::ostringstream out;
  for (int i = 0; i < 16; i++)
    out << std::setfill('0') << std::setw(2) << std::hex << int(hash[i]);
  return out.str();  
}

/* EOF */
#endif
