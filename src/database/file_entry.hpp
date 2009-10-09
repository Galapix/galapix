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

#ifndef HEADER_GALAPIX_DATABASE_FILE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_FILE_ENTRY_HPP

#include <boost/shared_ptr.hpp>

#include "util/url.hpp"
#include "math/math.hpp"
#include "math/size.hpp"

class FileEntryImpl
{
public:
  /** Unique id by which one can refer to this FileEntry, used in the
      'tile' table in the database */
  int64_t fileid;

  /** The URL of the image file */
  URL url;

  /** The size of the image in pixels */
  Size image_size;

  int file_size;
  int file_mtime;

  int thumbnail_size;

  FileEntryImpl()
    : fileid(),
      url(),
      image_size(),
      file_size(),
      file_mtime(),
      thumbnail_size()
  {}
};

class FileEntry 
{
private:
  FileEntry(int64_t fileid, 
            const URL& url,
            int size,
            int mtime,
            int width,
            int height)
    : impl(new FileEntryImpl())
  {
    impl->fileid     = fileid;
    impl->url        = url;
    impl->image_size = Size(width, height);
    impl->file_size  = size;
    impl->file_mtime = mtime;

    int s = Math::max(width, height);
    impl->thumbnail_size = 0;
    while(s > 8)
    {
      s /= 2;
      impl->thumbnail_size += 1;
    }
  }

public:
  FileEntry() :
    impl()
  {}

  static FileEntry create_without_fileid(const URL& url,
                                         int size,
                                         int mtime,
                                         int width,
                                         int height)
  {
    return FileEntry(0, url, size, mtime, width, height);
  }

  static FileEntry create(int64_t fileid, 
                          const URL& url,
                          int size,
                          int mtime,
                          int width,
                          int height)
  {
    return FileEntry(fileid, url, size, mtime, width, height);
  }

  bool        has_fileid() const { return impl->fileid != 0; }
  void        set_fileid(int64_t fileid) { impl->fileid = fileid; }
  int64_t     get_fileid()     const { return impl->fileid; }
  URL         get_url()        const { return impl->url; }
  int         get_width()      const { return impl->image_size.width; }
  int         get_height()     const { return impl->image_size.height; }
  Size        get_image_size() const { return impl->image_size; }

  int         get_size()  const { return impl->file_size;  }
  int         get_mtime() const { return impl->file_mtime; }

  int get_thumbnail_scale() const { return impl->thumbnail_size; }

  operator bool() const { return impl.get(); }
 
private:
  boost::shared_ptr<FileEntryImpl> impl;
};

std::ostream& operator<<(std::ostream& os, const FileEntry& entry);

#endif

/* EOF */
