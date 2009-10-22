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

#include <boost/shared_ptr.hpp>

#include "database/file_entry.hpp"

class Job;

class DatabaseMessage
{
public:
  DatabaseMessage()
  {}

  virtual ~DatabaseMessage()
  {}

  virtual void run(Database& db) =0;
};

class RequestJobRemovalDatabaseMessage : public DatabaseMessage
{
private:
  boost::shared_ptr<Job> m_job;

public:
  RequestJobRemovalDatabaseMessage(boost::shared_ptr<Job> job) :
    m_job(job)
  {}

  void run(Database& db)
  {
    DatabaseThread::current()->remove_job(m_job);
  }
};

class RequestTilesDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle m_job_handle;
  FileEntry m_file_entry;
  int m_min_scale;
  int m_max_scale;
  boost::function<void (TileEntry)> m_callback;

  RequestTilesDatabaseMessage(const JobHandle& job_handle, const FileEntry& file_entry, 
                              int min_scale, int max_scale,
                              const boost::function<void (TileEntry)>& callback) :
    m_job_handle(job_handle),
    m_file_entry(file_entry),
    m_min_scale(min_scale),
    m_max_scale(max_scale),
    m_callback(callback)
  {}

  void run(Database& db)
  {
    if (!m_job_handle.is_aborted())
    {
      DatabaseThread::current()->generate_tiles(m_job_handle,
                                                m_file_entry,
                                                m_min_scale, m_max_scale, 
                                                m_callback);
    }
  }
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
                             const boost::function<void (TileEntry)>& callback_) :
    job_handle(job_handle_),
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
      if (db.tiles.get_tile(file_entry, tilescale, pos, tile))
      {
        // Tile has been found, so return it and finish up
        if (callback)
        {
          callback(tile);
        }
        job_handle.set_finished();
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
        
        {
          DatabaseThread::current()->generate_tile(job_handle, file_entry, tilescale, pos, callback);
        }
      }
    }
  }
};

class RequestFileDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle m_job_handle;
  URL m_url;
  boost::function<void (FileEntry)> m_file_callback;
  boost::function<void (TileEntry)> m_tile_callback;

  RequestFileDatabaseMessage(const JobHandle& job_handle,
                             const URL& url,
                             const boost::function<void (FileEntry)>& file_callback,
                             const boost::function<void (TileEntry)>& tile_callback)
    : m_job_handle(job_handle),
      m_url(url),
      m_file_callback(file_callback),
      m_tile_callback(tile_callback)
  {}
  
  void run(Database& db)
  {
    if (!m_job_handle.is_aborted())
    {
      FileEntry entry = db.files.get_file_entry(m_url);
      if (!entry)
      {
        // file entry is not in the database, so try to generate it
        DatabaseThread::current()->generate_file_entry(m_job_handle, m_url, m_file_callback, m_tile_callback);
      }
      else
      {
        m_file_callback(entry);
        m_job_handle.set_finished();
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
    db.files.get_file_entries(m_pattern, entries);
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
                << tile.get_file_entry().get_fileid() << " pos: " 
                << tile.get_pos()    << " scale: " 
                << tile.get_scale()  << std::endl;

    // FIXME: Make some better error checking in case of loading failure
    if (tile.get_surface())
      {
        // FIXME: Test the performance of this
        //if (!db.tiles.has_tile(tile.fileid, tile.pos, tile.scale))
        db.tiles.store_tile(tile);
      }
    else
      {
        
      }
  }
};

class DeleteFileEntryDatabaseMessage : public DatabaseMessage
{
public:
  FileId m_fileid;

  DeleteFileEntryDatabaseMessage(const FileId& fileid)
    : m_fileid(fileid)
  {}

  void run(Database& db)
  {
    std::cout << "Begin Delete" << std::endl;
    db.get_db().exec("BEGIN;");
    db.files.delete_file_entry(m_fileid);
    db.tiles.delete_tiles(m_fileid);
    db.get_db().exec("END;");
    std::cout << "End Delete" << std::endl;
  }
};

class StoreFileEntryDatabaseMessage : public DatabaseMessage
{
private:
  JobHandle m_job_handle;
  URL  m_url;
  Size m_size;
  boost::function<void (FileEntry)> m_callback;

public:
  StoreFileEntryDatabaseMessage(const JobHandle& job_handle, 
                                const URL& url, const Size& size,
                                const boost::function<void (FileEntry)>& callback)
    : m_job_handle(job_handle),
      m_url(url),
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
    m_job_handle.set_finished();
  }
};


class ReceiveFileEntryDatabaseMessage : public DatabaseMessage
{
private:
  FileEntry m_file_entry;
  
public:
  ReceiveFileEntryDatabaseMessage(const FileEntry& file_entry) :
    m_file_entry(file_entry)
  {}

  void run(Database& db)
  {
    db.files.store_file_entry(m_file_entry);
  }
};

#endif

/* EOF */
