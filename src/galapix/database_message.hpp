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

#ifndef HEADER_GALAPIX_GALAPIX_DATABASE_MESSAGE_HPP
#define HEADER_GALAPIX_GALAPIX_DATABASE_MESSAGE_HPP

#include "database/file_entry.hpp"

class DatabaseMessage
{
public:
  DatabaseMessage()
  {}

  virtual ~DatabaseMessage()
  {}

  virtual void run(Database& db) =0;
};

class RequestTileDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle job_handle;

  FileEntry file_entry;
  int tilescale;
  Vector2i pos;
  boost::function<void (TileEntry)> callback;

  RequestTileDatabaseMessage(const JobHandle& job_handle_,
                             const FileEntry& file_entry_, int tilescale_, const Vector2i& pos_,
                             const boost::function<void (TileEntry)>& callback_)
    : job_handle(job_handle_),
      file_entry(file_entry_),
      tilescale(tilescale_),
      pos(pos_),
      callback(callback_)
  {}

  void run(Database& db)
  {
    if (!job_handle.is_aborted())
      {
        TileEntry tile;
        if (db.tiles.get_tile(file_entry.get_fileid(), tilescale, pos, tile))
          {
            // Tile has been found, so return it and finish up
            if (callback)
              callback(tile);
            job_handle.finish();
          }
        else
          {
            // Tile hasn't been found, so we need to generate it
            if (0)
              std::cout << "Error: Couldn't get tile: " 
                        << file_entry.get_fileid() << " "
                        << pos.x << " "
                        << pos.y << " "
                        << tilescale
                        << std::endl;
                          
            DatabaseThread::current()->generate_tile(job_handle, file_entry, tilescale, pos, callback);
          }
      }
  }
};

class RequestFileDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle job_handle;
  URL url;
  boost::function<void (FileEntry)> callback;

  RequestFileDatabaseMessage(const JobHandle& job_handle_,
                             const URL& url_,
                             const boost::function<void (FileEntry)>& callback_)
    : job_handle(job_handle_),
      url(url_),
      callback(callback_)
  {}

  void run(Database& db)
  {
    if (!job_handle.is_aborted())
      {
        FileEntry entry = db.files.get_file_entry(url);
        if (!entry)
          {
            DatabaseThread::current()->generate_file_entry(job_handle, url, callback);
            //std::cout << "Error: Couldn't get FileEntry for " << url << std::endl;
            //job_handle.finish();
          }
        else
          {
            callback(entry);
            job_handle.finish();
          }
      }
  }
};

class AllFilesDatabaseMessage : public DatabaseMessage
{
public:
  boost::function<void (FileEntry)> callback;

  AllFilesDatabaseMessage(const boost::function<void (FileEntry)>& callback_)
    : callback(callback_)
  {
  }

  void run(Database& db)
  {
    std::vector<FileEntry> entries;
    db.files.get_file_entries(entries);
    for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
      {
        callback(*i);
      }
  }
};

class FilesByPatternDatabaseMessage : public DatabaseMessage
{
public:
  std::string m_pattern;
  boost::function<void (FileEntry)> m_callback;

  FilesByPatternDatabaseMessage(const boost::function<void (FileEntry)>& callback, const std::string& pattern)
    : m_pattern(pattern),
      m_callback(callback)
  {
  }

  void run(Database& db)
  {
    std::vector<FileEntry> entries;
    db.files.get_file_entries(entries, m_pattern);
    for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
      {
        m_callback(*i);
      }               
  }
};

class ReceiveTileDatabaseMessage : public DatabaseMessage
{
public:
  TileEntry tile;

  ReceiveTileDatabaseMessage(const TileEntry& tile_)
    : tile(tile_)
  {}

  void run(Database& db)
  {
    if (0)
      std::cout << "Received Tile: "
                << tile.get_fileid() << " pos: " 
                << tile.get_pos()    << " scale: " 
                << tile.get_scale()  << std::endl;

    DatabaseThread::current()->process_tile(tile);

    // FIXME: Make some better error checking in case of loading failure
    if (tile.get_surface())
      {
        // Avoid doubble store of tiles in the database
        // FIXME: Test the performance of this
        //if (!db.tiles.has_tile(tile.fileid, tile.pos, tile.scale))
        db.tiles.store_tile_in_cache(tile);
      }
    else
      {
        
      }
  }
};

class DeleteFileEntryDatabaseMessage : public DatabaseMessage
{
public:
  int64_t fileid;

  DeleteFileEntryDatabaseMessage(int64_t fileid_)
    : fileid(fileid_)
  {}

  void run(Database& db)
  {
    SQLiteStatement(&db.db)
      .prepare("BEGIN;")
      .execute();
    SQLiteStatement(&db.db)
      .prepare("DELETE FROM files WHERE fileid = ?1;")
      .bind_int64(1, fileid)
      .execute();
    SQLiteStatement(&db.db)
      .prepare("DELETE FROM tiles WHERE fileid = ?1;")
      .bind_int64(1, fileid)
      .execute();
    SQLiteStatement(&db.db)
      .prepare("END;")
      .execute();
  }
};

class StoreFileEntryDatabaseMessage : public DatabaseMessage
{
private:
  URL  m_url;
  Size m_size;
  boost::function<void (FileEntry)> m_callback;

public:
  StoreFileEntryDatabaseMessage(const URL& url, const Size& size,
                                const boost::function<void (FileEntry)>& callback)
    : m_url(url),
      m_size(size),
      m_callback(callback)
  {}

  void run(Database& db)
  {
    FileEntry file_entry = db.files.store_file_entry(m_url, m_size);
    if (m_callback)
    {
      m_callback(file_entry);
    }
  }
};

#endif

/* EOF */
