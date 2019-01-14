/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include <assert.h>
#include <logmich/log.hpp>

#include "database/entries/image_entry.hpp"
#include "galapix/tile_provider.hpp"
#include "server/database_thread.hpp"

class DatabaseTileProvider : public TileProvider
{
public:
  DatabaseTileProvider(const OldFileEntry& file_entry,
                       const ImageEntry& image_entry) :
    m_file_entry(file_entry),
    m_image_entry(image_entry)
  {}

  JobHandle request_tile(int tilescale, const Vector2i& pos,
                         const std::function<void (Tile)>& callback) override
  {
    return DatabaseThread::current()->request_tile(m_file_entry, tilescale, pos, callback);
  }

  int get_max_scale() const override
  {
    return m_image_entry.get_max_scale();
  }

  int get_tilesize() const override
  {
    return 256;
  }

  Size get_size() const override
  {
    return m_image_entry.get_size();
  }

  void refresh(const std::function<void (TileProviderPtr)>& callback) override
  {
#if 0
    DatabaseThread::current()->delete_file_entry(m_file_entry.get_fileid());
    DatabaseThread::current()->request_file(m_file_entry.get_url(),
                                            [=](){
                                              callback(DatabaseTileProvider::create(file_entry));
                                            });
#else
    log_error("not implemented");
#endif
  }

private:
  OldFileEntry m_file_entry;
  ImageEntry m_image_entry;

private:
  DatabaseTileProvider(const DatabaseTileProvider&) = delete;
  DatabaseTileProvider& operator=(const DatabaseTileProvider&) = delete;
};

#endif

/* EOF */
