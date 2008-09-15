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

#ifndef HEADER_IMAGE_HPP
#define HEADER_IMAGE_HPP

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>
#include "job_handle.hpp"
#include "math/vector2f.hpp"
#include "math/size.hpp"
#include "grid.hpp"
#include "surface.hpp"

class Surface;
class Size;
class Rectf;
class Vector2f;
class ImageImpl;
class FileEntry;
class TileEntry;

class Image
{
public:
  struct SurfaceStruct {
    enum Status { SURFACE_OK,
                  SURFACE_REQUESTED,
                  SURFACE_FAILED };

    Status  status;
    Surface surface;
  };

  typedef std::map<uint32_t, SurfaceStruct> Cache; 
  typedef std::vector<JobHandle> Jobs;

private:
  Surface get_tile(int x, int y, int tile_scale);

  void process_queue();
  Surface find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_in);

public:
  Image();
  Image(const FileEntry& file_entry);

  void draw_tile(int x, int y, int tiledb_scale, const Vector2f& rect, float scale);
  void draw_tiles(const Rect& rect, int tiledb_scale, const Vector2f& pos, float scale);
  void draw(const Rectf& cliprect, float scale);
  void draw_mark();

  void update_pos(float progress);

  void  set_alpha(float alpha);
  float get_alpha() const;

  void set_target_pos(const Vector2f& target_pos);
  void set_target_scale(float target_scale);

  Vector2f get_top_left_pos() const;

  void     set_pos(const Vector2f& pos);
  Vector2f get_pos() const;

  void  set_scale(float f);
  float get_scale() const;

  void  set_angle(float a);
  float get_angle(float a) const;

  float get_scaled_width() const;
  float get_scaled_height() const;

  int get_original_width() const;
  int get_original_height() const;

  void clear_cache();
  void cache_cleanup();
  void print_info();

  URL get_url() const;

  bool overlaps(const Rectf& cliprect) const;
  bool overlaps(const Vector2f& pos) const;

  Rectf get_image_rect() const;

  operator bool() const { return impl.get(); }
  bool operator==(const Image& rhs) const { 
    // FIXME: Shouldn't really use operator==, should use equal() instead
    return impl.get() == rhs.impl.get(); 
  }

  /** Syncronized function to require data from other threads */
  void receive_tile(const TileEntry& tile_entry);

private:
  boost::shared_ptr<ImageImpl> impl;
};

#endif

/* EOF */
