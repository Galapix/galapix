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

#ifndef HEADER_FILE_ENTRY_HPP
#define HEADER_FILE_ENTRY_HPP

#include <stdint.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include "url.hpp"
#include "math.hpp"
#include "math/size.hpp"
#include "math/rgb.hpp"
#include "software_surface.hpp"

class FileEntryImpl
{
public:
  /** Unique id by which one can refer to this FileEntry, used in the
      'tile' table in the database */
  uint32_t fileid;

  /** The URL of the image file */
  URL url;

  /** The size of the image in pixels */
  Size size;

  int thumbnail_size;
};

class FileEntry 
{
public:
  FileEntry()
  {}

  FileEntry(uint32_t fileid, 
            const URL& url,
            int width,
            int height)
    : impl(new FileEntryImpl())
  {
    impl->fileid    = fileid;
    impl->url       = url;
    impl->size      = Size(width, height);

    int s = Math::max(width, height);
    impl->thumbnail_size = 0;
    while(s > 8)
      {
        s /= 2;
        impl->thumbnail_size += 1;
      }
  }

  uint32_t    get_fileid()   const { return impl->fileid; }
  URL         get_url()      const { return impl->url; }
  std::string get_filename() const { return impl->url.get_stdio_name(); } // FIXME: Remove this
  int         get_width()    const { return impl->size.width; }
  int         get_height()   const { return impl->size.height; }
  Size        get_size()     const { return impl->size; }

  int get_thumbnail_scale() const { return impl->thumbnail_size; }

  operator bool() const { return impl.get(); }
 
private:
  boost::shared_ptr<FileEntryImpl> impl;
};

std::ostream& operator<<(std::ostream& os, const FileEntry& entry);

#endif

/* EOF */
