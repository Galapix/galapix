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

#ifndef HEADER_GALAPIX_GALAPIX_DATABASE_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_GALAPIX_DATABASE_TILE_PROVIDER_HPP

#include "galapix/tile_provider.hpp"

class DatabaseTileProvider : public TileProvider
{
private:
  FileEntry m_file_entry;

private:
  DatabaseTileProvider(const FileEntry& file_entry) :
    m_file_entry(file_entry)
  {}

public:
  static boost::shared_ptr<DatabaseTileProvider> create(const FileEntry& file_entry)
  {
    return boost::shared_ptr<DatabaseTileProvider>(new DatabaseTileProvider(file_entry));
  }

  JobHandle request_tile(int tilescale, const Vector2i& pos, 
                         const boost::function<void (Tile)>& callback)
  {
    return DatabaseThread::current()->request_tile(m_file_entry, tilescale, pos, callback);
  }
  
  int get_max_scale() const 
  {
    return m_file_entry.get_thumbnail_scale(); 
  }

  int get_tilesize() const 
  {
    return 256;
  }
  
  int get_overlap() const 
  {
    return 0;
  }

  Size get_size() const
  {
    return m_file_entry.get_image_size();
  }

  struct FileEntry2TileProvider
  {
    boost::function<void (TileProviderPtr)> m_callback;

    FileEntry2TileProvider(const boost::function<void (TileProviderPtr)>& callback) :
      m_callback(callback)
    {}

    void operator()(const FileEntry& file_entry)
    {
      m_callback(DatabaseTileProvider::create(file_entry));
    }
  };

  void refresh(const boost::function<void (TileProviderPtr)>& callback)
  {
    DatabaseThread::current()->delete_file_entry(m_file_entry.get_fileid());
    DatabaseThread::current()->request_file(m_file_entry.get_url(), 
                                            FileEntry2TileProvider(callback));
  }

private:
  DatabaseTileProvider(const DatabaseTileProvider&);
  DatabaseTileProvider& operator=(const DatabaseTileProvider&);
};

#endif

/* EOF */
