/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmx.de>
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

#include "database/file_tile_database.hpp"

#include <boost/format.hpp>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <limits>

#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "util/blob.hpp"
#include "util/filesystem.hpp"
#include "util/software_surface_factory.hpp"
#include "galapix/tile.hpp"

FileTileDatabase::FileTileDatabase(const std::string& prefix) :
  m_prefix(prefix)
{
  Filesystem::mkdir(prefix);
}

FileTileDatabase::~FileTileDatabase()
{
}

bool
FileTileDatabase::has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  return Filesystem::exist(get_complete_filename(file_entry, pos, scale));
}

bool
FileTileDatabase::get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile_out)
{
  std::string filename = get_complete_filename(file_entry, pos, scale);
  if (Filesystem::exist(filename))
  {
    SoftwareSurfacePtr surface;
    if (file_entry.get_format() == FileEntry::JPEG_FORMAT)
    {
      surface = JPEG::load_from_file(filename);
    }
    else
    {
      surface = PNG::load_from_file(filename);
    }

    tile_out = TileEntry(file_entry, scale, pos, surface);
    //Blob::from_file(filename), 
    //static_cast<TileEntry::Format>(file_entry.get_format())); // FIXME: should unify format
    return true;
  }
  else
  {
    return false;
  }
}

void
FileTileDatabase::get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles)
{
  std::string directory = get_complete_directory(file_entry.get_fileid());
  std::vector<std::string> files = Filesystem::open_directory(directory);
  for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
  {
    Vector2i pos;
    int scale;
    int format;

    if (!parse_filename(*i, &pos, &scale, &format))
    {
      std::cout << "FileTileDatabase::get_tiles(): unknown file in database: " << directory << '/' << *i;
    }
    else
    {
      SoftwareSurfacePtr surface;
      if (file_entry.get_format() == FileEntry::JPEG_FORMAT)
      {
        surface = JPEG::load_from_file(directory + '/' + *i);
      }
      else
      {
        surface = PNG::load_from_file(directory + '/' + *i);
      }

      tiles.push_back(TileEntry(file_entry, scale, pos, surface));
    }
  }
}

bool
FileTileDatabase::get_min_max_scale(const FileEntry& file_entry, int& min_scale_out, int& max_scale_out)
{
  min_scale_out = std::numeric_limits<int>::max();
  max_scale_out = std::numeric_limits<int>::min();

  std::string directory = get_complete_directory(file_entry.get_fileid());
  std::vector<std::string> files = Filesystem::open_directory(directory);
  for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
  {
    Vector2i pos;
    int scale;
    int format;

    if (!parse_filename(*i, &pos, &scale, &format))
    {
      std::cout << "FileTileDatabase::get_tiles(): unknown file in database: " << directory << '/' << *i;
    }
    else
    {
      min_scale_out = std::min(min_scale_out, scale);
      max_scale_out = std::max(max_scale_out, scale);
    }
  }

  if (min_scale_out == std::numeric_limits<int>::max() ||
      max_scale_out == std::numeric_limits<int>::min())
  {
    return false;
  }
  else
  {
    return true;
  }
}

void
FileTileDatabase::store_tile(const FileEntry& file_entry, const Tile& tile)
{ 
  // Ensure that the directory exists, FIX
  ensure_directory_exists(file_entry.get_fileid());

  std::string filename = get_complete_filename(file_entry, tile.get_pos(), tile.get_scale());

  std::cout << "Saving to: " << filename << std::endl;

  switch(tile.get_surface()->get_format())
  {
    case SoftwareSurface::RGB_FORMAT:
      JPEG::save(tile.get_surface(), 75, filename);
      break;

    case SoftwareSurface::RGBA_FORMAT:
      PNG::save(tile.get_surface(), filename);
      break;
          
    default:
      assert(!"Never reached");
  }
}

void
FileTileDatabase::store_tiles(const std::vector<TileEntry>& tiles)
{
  for(std::vector<TileEntry>::const_iterator i = tiles.begin(); i != tiles.end(); ++i)
  {
    store_tile(i->get_file_entry(), Tile(*i));
  }
}

void
FileTileDatabase::delete_tiles(const FileId& fileid)
{
  // Filesystem::remove()
  // get_complete_filename()
  // FIXME: implement me
}

std::string
FileTileDatabase::get_directory(const FileId& file_id_obj)
{
  int64_t file_id = static_cast<int>(file_id_obj.get_id());
  
  // FIXME: Ignoring the last 32 bits for now
  int part1 = (file_id >> 24) & 0xfff;
  int part2 = (file_id >> 12) & 0xfff;
  int part3 = (file_id >>  0) & 0xfff;

  return (boost::format("%03x/%03x/%03x") % part1 % part2 % part3).str();
}

std::string
FileTileDatabase::get_filename(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  std::string ext = "jpg";

  if (file_entry.get_format() == FileEntry::PNG_FORMAT)
    ext = "png";

  return (boost::format("tile-%03d-%03d-%03d.%s") % scale % pos.x % pos.y % ext).str();
}

std::string
FileTileDatabase::get_complete_filename(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  std::ostringstream str(m_prefix);
  str << m_prefix << '/' << get_directory(file_entry.get_fileid()) << '/' << get_filename(file_entry, pos, scale);
  return str.str();
}

std::string
FileTileDatabase::get_complete_directory(const FileId& file_id)
{
  return m_prefix + "/" + get_directory(file_id);
}

bool
FileTileDatabase::parse_filename(const std::string& filename, Vector2i* pos_out, int* scale_out, int* format_out)
{
  char format[4];
  if (sscanf(filename.c_str(), "tile-%d-%d-%d.%3s", scale_out, &pos_out->x, &pos_out->y, format) == 3)
  {
    if (strcmp(format, "jpg"))
    {
      *format_out = FileEntry::JPEG_FORMAT;
    }
    else if (strcmp(format, "png"))
    {
      *format_out = FileEntry::PNG_FORMAT;
    }
    else
    {
      return false;
    }

    return true;
  }
  else
  {
    return false;
  }
}

void
FileTileDatabase::ensure_directory_exists(const FileId& file_id_obj)
{
  int64_t file_id = static_cast<int>(file_id_obj.get_id());
  
  // FIXME: Ignoring the last 32 bits for now
  int part1 = (file_id >> 24) & 0xfff;
  int part2 = (file_id >> 12) & 0xfff;
  int part3 = (file_id >>  0) & 0xfff;

  std::ostringstream str;
  str << m_prefix;

  str << '/' << boost::format("%03x") % part1;
  Filesystem::mkdir(str.str());

  str << '/' << boost::format("%03x") % part2;
  Filesystem::mkdir(str.str());

  str << '/' << boost::format("%03x") % part3;
  Filesystem::mkdir(str.str());
}

/* EOF */
