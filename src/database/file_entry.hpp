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

#include <memory>
#include <assert.h>

#include "database/file_id.hpp"
#include "math/math.hpp"
#include "math/size.hpp"
#include "util/url.hpp"

class FileEntryImpl
{
public:
  /** Unique id by which one can refer to this FileEntry, used in the
      'tile' table in the database */
  FileId fileid;

  /** The URL of the image file */
  URL url;

  /** The size of the image in pixels */
  Size image_size;
  int  format;

  int file_size;
  int file_mtime;

  int thumbnail_size;

  FileEntryImpl() :
    fileid(),
    url(),
    image_size(),
    format(),
    file_size(),
    file_mtime(),
    thumbnail_size()
  {}
};

class FileEntry 
{
private:
  FileEntry(const FileId& fileid,
            const URL& url,
            int size,
            int mtime,
            int width,
            int height,
            int format) :
    impl(new FileEntryImpl())
  {
    impl->fileid     = fileid;
    impl->url        = url;
    impl->image_size = Size(width, height);
    impl->file_size  = size;
    impl->format     = format;
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
  enum Format 
  {
    UNKNOWN_FORMAT = -1,
    JPEG_FORMAT =  0,
    PNG_FORMAT  =  1
  };


  FileEntry() :
    impl()
  {}

  static FileEntry create_without_fileid(const URL& url,
                                         int size,
                                         int mtime,
                                         int width,
                                         int height,
                                         int format)
  {
    return FileEntry(FileId(), url, size, mtime, width, height, format);
  }

  static FileEntry create(const FileId& fileid, 
                          const URL& url,
                          int size,
                          int mtime,
                          int width,
                          int height,
                          int format)
  {
    return FileEntry(fileid, url, size, mtime, width, height, format);
  }

  void        set_fileid(const FileId& fileid) { impl->fileid = fileid; }
  FileId      get_fileid()     const { return impl->fileid; }
  URL         get_url()        const { return impl->url; }
  int         get_width()      const { return impl->image_size.width; }
  int         get_height()     const { return impl->image_size.height; }
  Size        get_image_size() const { return impl->image_size; }
  int         get_format()     const { return impl->format; }

  int         get_size()  const { return impl->file_size;  }
  int         get_mtime() const { return impl->file_mtime; }

  int get_thumbnail_scale() const { return impl->thumbnail_size; }

  operator void*() const { return impl.get(); }
  bool operator==(const FileEntry& rhs) const
  {
    if (impl->fileid && rhs.impl->fileid)
    {
      return get_fileid() == rhs.get_fileid();
    }
    else if (impl == rhs.impl)
    {
      return true;
    }
    else
    {
      // FIXME: Not quite, as width/height might differ
      return impl->url == rhs.impl->url;
    }
  }

private:
  std::shared_ptr<FileEntryImpl> impl;
};

std::ostream& operator<<(std::ostream& os, const FileEntry& entry);

#endif

/* EOF */
